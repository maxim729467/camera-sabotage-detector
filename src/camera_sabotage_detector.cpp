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

        // Create result object
        Napi::Object result = Napi::Object::New(env);
        result.Set("blurScore", Napi::Number::New(env, blurScore));
        result.Set("blackoutScore", Napi::Number::New(env, blackoutScore));
        result.Set("flashScore", Napi::Number::New(env, flashScore));

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

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(
        Napi::String::New(env, "detectSabotage"),
        Napi::Function::New(env, DetectSabotage)
    );
    exports.Set(
        Napi::String::New(env, "detectSceneChange"),
        Napi::Function::New(env, DetectSceneChange)
    );
    return exports;
}

NODE_API_MODULE(sabotage_detector, Init) 