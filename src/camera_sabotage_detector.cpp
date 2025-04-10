#include <napi.h>
#include <opencv2/opencv.hpp>
#include <vector>

// Helper function to read image from buffer
cv::Mat readImageFromBuffer(const Napi::Buffer<uint8_t>& buffer) {
    std::vector<uint8_t> vec(buffer.Data(), buffer.Data() + buffer.Length());
    return cv::imdecode(vec, cv::IMREAD_GRAYSCALE);
}

// Helper function to calculate blur score
double calculateBlurScore(const cv::Mat& gray) {
    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);
    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian, mean, stddev);
    double variance = stddev[0] * stddev[0];
    const double MIN_VARIANCE = 0.0;
    const double MAX_VARIANCE = 1000.0;
    return 100.0 - std::min(100.0, std::max(0.0, 
        ((variance - MIN_VARIANCE) / (MAX_VARIANCE - MIN_VARIANCE)) * 100.0
    ));
}

// Helper function to calculate blackout score
double calculateBlackoutScore(const cv::Mat& gray) {
    double avgIntensity = cv::mean(gray)[0];
    
    // Calculate percentage of dark pixels
    cv::Mat hist;
    int histSize = 256;
    float range[] = {0, 256};
    const float* histRange = {range};
    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    
    double darkPixels = 0;
    for (int i = 0; i < 75; i++) {  // Count pixels with intensity 0-74
        darkPixels += hist.at<float>(i);
    }
    double totalPixels = gray.rows * gray.cols;
    double darkPercentage = (darkPixels / totalPixels) * 100.0;
    
    // Moderately sensitive blackout detection
    // Detect blackout when average intensity is below 60
    // and there's a moderate percentage of dark pixels
    double intensityScore = std::max(0.0, (60.0 - avgIntensity) * 1.5);
    double darkPixelScore = darkPercentage * 0.6;
    return std::min(100.0, intensityScore + darkPixelScore);
}

// Helper function to calculate flash score
double calculateFlashScore(const cv::Mat& gray) {
    cv::Mat hist;
    int histSize = 256;
    float range[] = {0, 256};
    const float* histRange = {range};
    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    
    // Calculate percentage of bright pixels
    double highIntensityPixels = 0;
    for (int i = 200; i < 256; i++) {
        highIntensityPixels += hist.at<float>(i);
    }
    double totalPixels = gray.rows * gray.cols;
    double brightPercentage = (highIntensityPixels / totalPixels) * 100.0;
    
    // Convert to 0-100 scale where higher means more flash
    return std::min(100.0, std::max(0.0, brightPercentage * 3.0));
}

// Helper function to calculate scene change score
double calculateSceneChangeScore(const cv::Mat& current, const cv::Mat& previous) {
    if (previous.empty()) return 0.0;
    
    cv::Mat diff;
    cv::absdiff(current, previous, diff);
    double avgDiff = cv::mean(diff)[0];
    
    // Convert to 0-100 scale where higher means more change
    // Assuming significant change starts at 50.0 difference
    return std::min(100.0, std::max(0.0, (avgDiff / 50.0) * 100.0));
}

Napi::Object DetectSabotage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    cv::Mat gray;

    try {
        if (info[0].IsString()) {
            // Handle file path
            std::string imagePath = info[0].As<Napi::String>().Utf8Value();
            gray = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
        } else if (info[0].IsBuffer()) {
            // Handle buffer
            Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
            gray = readImageFromBuffer(buffer);
        } else {
            Napi::TypeError::New(env, "Expected string or buffer argument").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        if (gray.empty()) {
            Napi::Error::New(env, "Failed to read image").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        // Calculate all scores
        double blurScore = calculateBlurScore(gray);
        double blackoutScore = calculateBlackoutScore(gray);
        double flashScore = calculateFlashScore(gray);

        // Calculate smear score
        double smearScore = 0.0;

        // Calculate global contrast and brightness
        cv::Scalar meanIntensity, stddevIntensity;
        cv::meanStdDev(gray, meanIntensity, stddevIntensity);
        double brightness = meanIntensity[0];
        double contrastScore = 100.0 - std::min(100.0, std::max(0.0, (stddevIntensity[0] / 10.0) * 100.0));

        // Calculate edge density
        cv::Mat edges;
        cv::Canny(gray, edges, 50, 150);
        double edgeDensity = cv::countNonZero(edges) / (double)(edges.rows * edges.cols);
        double edgeScore = 100.0 - std::min(100.0, edgeDensity * 150.0);

        // Calculate intensity histogram
        cv::Mat hist;
        int histSize = 256;
        float range[] = {0, 256};
        const float* histRange = {range};
        cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);

        // Calculate percentage of different intensity ranges
        double darkPixels = 0, midPixels = 0, brightPixels = 0;
        for (int i = 0; i < 256; i++) {
            if (i < 85) darkPixels += hist.at<float>(i);
            else if (i < 170) midPixels += hist.at<float>(i);
            else brightPixels += hist.at<float>(i);
        }
        double totalPixels = gray.rows * gray.cols;
        double darkPercentage = (darkPixels / totalPixels) * 100.0;
        double midPercentage = (midPixels / totalPixels) * 100.0;
        double brightPercentage = (brightPixels / totalPixels) * 100.0;

        // Calculate base characteristics score with adjusted weights
        double baseScore = (blurScore * 0.5) + (contrastScore * 0.3) + (edgeScore * 0.2);

        // Calculate intensity distribution score with adjusted thresholds
        double intensityScore = 0.0;
        
        // Adjust thresholds based on overall brightness
        double brightnessFactor = std::min(1.0, brightness / 120.0);
        double darkThreshold = 8.0 + (brightnessFactor * 3.0);
        double brightThreshold = 8.0 + ((1.0 - brightnessFactor) * 3.0);
        double midThreshold = 15.0 + (brightnessFactor * 2.0);

        // Increase sensitivity to bright conditions
        if (brightness > 120.0) {
            intensityScore += (brightness - 120.0) * 0.8;
        }

        if (darkPercentage > darkThreshold) intensityScore += darkPercentage * 0.5;
        if (brightPercentage > brightThreshold) intensityScore += brightPercentage * 0.5;
        if (midPercentage > midThreshold) intensityScore += midPercentage * 0.3;

        // Calculate combined score
        double combinedScore = baseScore + (intensityScore * 0.4);

        // Invert the scoring logic - higher scores for smears, lower for normal images
        if (combinedScore > 20.0) { // Lower threshold to catch more smears
            // Give high scores for smears
            smearScore = std::min(100.0, 20.0 + (combinedScore - 20.0) * 1.5);
        } else {
            // Give low scores for normal images
            smearScore = combinedScore * 0.5;
        }

        // Create result object
        Napi::Object result = Napi::Object::New(env);
        result.Set("blurScore", Napi::Number::New(env, blurScore));
        result.Set("blackoutScore", Napi::Number::New(env, blackoutScore));
        result.Set("flashScore", Napi::Number::New(env, flashScore));
        result.Set("smearScore", Napi::Number::New(env, smearScore));

        return result;
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }
}

Napi::Object DetectSceneChange(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    cv::Mat current, previous;

    try {
        // Handle current frame
        if (info[0].IsString()) {
            std::string currentImagePath = info[0].As<Napi::String>().Utf8Value();
            current = cv::imread(currentImagePath, cv::IMREAD_GRAYSCALE);
        } else if (info[0].IsBuffer()) {
            Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
            current = readImageFromBuffer(buffer);
        } else {
            Napi::TypeError::New(env, "Expected string or buffer argument for current frame").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        // Handle previous frame
        if (info[1].IsString()) {
            std::string previousImagePath = info[1].As<Napi::String>().Utf8Value();
            previous = cv::imread(previousImagePath, cv::IMREAD_GRAYSCALE);
        } else if (info[1].IsBuffer()) {
            Napi::Buffer<uint8_t> buffer = info[1].As<Napi::Buffer<uint8_t>>();
            previous = readImageFromBuffer(buffer);
        } else {
            Napi::TypeError::New(env, "Expected string or buffer argument for previous frame").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        if (current.empty() || previous.empty()) {
            Napi::Error::New(env, "Failed to read images").ThrowAsJavaScriptException();
            return Napi::Object::New(env);
        }

        double sceneChangeScore = calculateSceneChangeScore(current, previous);

        Napi::Object result = Napi::Object::New(env);
        result.Set("sceneChangeScore", Napi::Number::New(env, sceneChangeScore));
        return result;
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }
}

Napi::Object DetectSmear(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Object result = Napi::Object::New(env);

    // Validate input
    if (info.Length() < 1) {
        Napi::Error::New(env, "Expected an image path or buffer").ThrowAsJavaScriptException();
        result.Set("error", Napi::String::New(env, "Expected an image path or buffer"));
        return result;
    }

    // Read input image
    cv::Mat image;
    if (info[0].IsString()) {
        std::string imagePath = info[0].As<Napi::String>().Utf8Value();
        image = cv::imread(imagePath);
    } else if (info[0].IsBuffer()) {
        Napi::Buffer<uint8_t> buffer = info[0].As<Napi::Buffer<uint8_t>>();
        std::vector<uint8_t> vec(buffer.Data(), buffer.Data() + buffer.Length());
        image = cv::imdecode(vec, cv::IMREAD_COLOR);
    } else {
        Napi::Error::New(env, "Input must be a string (file path) or buffer").ThrowAsJavaScriptException();
        result.Set("error", Napi::String::New(env, "Input must be a string (file path) or buffer"));
        return result;
    }

    if (image.empty()) {
        Napi::Error::New(env, "Could not read image").ThrowAsJavaScriptException();
        result.Set("error", Napi::String::New(env, "Could not read image"));
        return result;
    }

    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Calculate global blur score
    double blurScore = calculateBlurScore(gray);

    // Calculate global contrast and brightness
    cv::Scalar meanIntensity, stddevIntensity;
    cv::meanStdDev(gray, meanIntensity, stddevIntensity);
    double brightness = meanIntensity[0];
    double contrastScore = 100.0 - std::min(100.0, std::max(0.0, (stddevIntensity[0] / 10.0) * 100.0));

    // Calculate edge density
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);
    double edgeDensity = cv::countNonZero(edges) / (double)(edges.rows * edges.cols);
    double edgeScore = 100.0 - std::min(100.0, edgeDensity * 150.0);

    // Calculate intensity histogram
    cv::Mat hist;
    int histSize = 256;
    float range[] = {0, 256};
    const float* histRange = {range};
    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);

    // Calculate percentage of different intensity ranges
    double darkPixels = 0, midPixels = 0, brightPixels = 0;
    for (int i = 0; i < 256; i++) {
        if (i < 85) darkPixels += hist.at<float>(i);
        else if (i < 170) midPixels += hist.at<float>(i);
        else brightPixels += hist.at<float>(i);
    }
    double totalPixels = gray.rows * gray.cols;
    double darkPercentage = (darkPixels / totalPixels) * 100.0;
    double midPercentage = (midPixels / totalPixels) * 100.0;
    double brightPercentage = (brightPixels / totalPixels) * 100.0;

    // Calculate smear score based on combined characteristics
    double smearScore = 0.0;

    // Calculate base characteristics score with adjusted weights
    double baseScore = (blurScore * 0.5) + (contrastScore * 0.3) + (edgeScore * 0.2);

    // Calculate intensity distribution score with adjusted thresholds
    double intensityScore = 0.0;
    
    // Adjust thresholds based on overall brightness
    double brightnessFactor = std::min(1.0, brightness / 120.0);
    double darkThreshold = 8.0 + (brightnessFactor * 3.0);
    double brightThreshold = 8.0 + ((1.0 - brightnessFactor) * 3.0);
    double midThreshold = 15.0 + (brightnessFactor * 2.0);

    // Increase sensitivity to bright conditions
    if (brightness > 120.0) {
        intensityScore += (brightness - 120.0) * 0.8;
    }

    if (darkPercentage > darkThreshold) intensityScore += darkPercentage * 0.5;
    if (brightPercentage > brightThreshold) intensityScore += brightPercentage * 0.5;
    if (midPercentage > midThreshold) intensityScore += midPercentage * 0.3;

    // Calculate combined score
    double combinedScore = baseScore + (intensityScore * 0.4);

    // Invert the scoring logic - higher scores for smears, lower for normal images
    if (combinedScore > 20.0) { // Lower threshold to catch more smears
        // Give high scores for smears
        smearScore = std::min(100.0, 20.0 + (combinedScore - 20.0) * 1.5);
    } else {
        // Give low scores for normal images
        smearScore = combinedScore * 0.5;
    }

    // Set result property
    result.Set("smearScore", Napi::Number::New(env, smearScore));

    return result;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(
        Napi::String::New(env, "detectSabotage"),
        Napi::Function::New(env, DetectSabotage)
    );
    exports.Set(
        Napi::String::New(env, "detectSceneChange"),
        Napi::Function::New(env, DetectSceneChange)
    );
    exports.Set(
        Napi::String::New(env, "detectSmear"),
        Napi::Function::New(env, DetectSmear)
    );
    return exports;
}

NODE_API_MODULE(sabotage_detector, Init) 