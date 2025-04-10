# Sabotage Detector

A Node.js library for detecting camera sabotage and scene changes in images and video frames. The library provides non-blocking, asynchronous detection capabilities using worker threads for optimal performance.

## Features

- **Camera Sabotage Detection**
  - Blur detection
  - Blackout detection
  - Flash detection
- **Scene Change Detection**
  - Frame-to-frame comparison
  - Change score calculation

## System Requirements

- OpenCV 4.x must be installed on your system
  - On macOS: `brew install opencv`
  - On Linux: `sudo apt-get install libopencv-dev` (Ubuntu/Debian)
- Node.js 14.x or higher
- Native dependencies (will be installed automatically)

## Platform Compatibility

This package has been tested and is supported on:

- macOS (Intel and Apple Silicon)
- Linux (Ubuntu/Debian)

Windows support is not currently available.

## Installation

```bash
npm install camera-sabotage-detector
```

## Usage

### Detecting Camera Sabotage

```javascript
const { detectSabotage } = require('camera-sabotage-detector');

// Using async/await
async function checkImage(imageBuffer) {
  try {
    const result = await detectSabotage(imageBuffer);
    console.log('Blur Score:', result.blurScore);
    console.log('Blackout Score:', result.blackoutScore);
    console.log('Flash Score:', result.flashScore);
  } catch (error) {
    console.error('Detection failed:', error);
  }
}

// Using Promises
detectSabotage(imageBuffer)
  .then((result) => {
    console.log('Detection results:', result);
  })
  .catch((error) => {
    console.error('Detection failed:', error);
  });
```

### Detecting Scene Changes

```javascript
const { detectSceneChange } = require('camera-sabotage-detector');

async function compareFrames(currentFrame, previousFrame) {
  try {
    const result = await detectSceneChange(currentFrame, previousFrame);
    console.log('Scene Change Score:', result.sceneChangeScore);
  } catch (error) {
    console.error('Scene change detection failed:', error);
  }
}
```

## API Reference

### `detectSabotage(input)`

Detects various types of camera sabotage in an image.

**Parameters:**

- `input` (string|Buffer): Image file path or buffer containing image data

**Returns:**
Promise resolving to an object containing:

- `blurScore` (number): 0-100 score indicating blur level
- `blackoutScore` (number): 0-100 score indicating blackout level
- `flashScore` (number): 0-100 score indicating flash level

### `detectSceneChange(current, previous)`

Detects significant changes between two consecutive frames.

**Parameters:**

- `current` (string|Buffer): Current frame image path or buffer
- `previous` (string|Buffer): Previous frame image path or buffer

**Returns:**
Promise resolving to an object containing:

- `sceneChangeScore` (number): 0-100 score indicating scene change level

## Performance

The library uses worker threads to ensure non-blocking operation, allowing your application to remain responsive while performing detection tasks. Multiple detection calls can run concurrently.

## Error Handling

All functions return Promises that will reject with an error if:

- The input image is invalid or corrupted
- The native detection fails
- Any other error occurs during processing

## License

MIT
