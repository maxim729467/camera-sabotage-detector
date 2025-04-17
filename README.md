# Camera Sabotage Detector

A Node.js module for detecting various types of camera sabotage and anomalies in video frames.

## Features

- **Defocus Detection**: Identifies when the camera is out of focus or has been smeared
- **Blackout Detection**: Detects when the camera has been covered or is experiencing low light conditions
- **Flash Detection**: Identifies sudden bright flashes that might indicate tampering
- **Scene Change Detection**: Monitors for significant changes between consecutive frames
- **Smear Detection**: Detects when the camera lens has been smeared or covered with a semi-transparent substance

## Installation

```bash
npm install camera-sabotage-detector
```

## Usage

```javascript
const { detectSabotage, detectSceneChange } = require('camera-sabotage-detector');

// Detect various types of sabotage in a single frame
const result = await detectSabotage('path/to/image.jpg');
// or
const result = await detectSabotage(imageBuffer);

console.log(result);
// {
//   defocusScore: 0-100,      // Higher score means more defocus
//   blackoutScore: 0-100,  // Higher score means more blackout
//   flashScore: 0-100,     // Higher score means more flash
//   smearScore: 0-100      // Higher score means more severe smears
// }

// Detect scene changes between consecutive frames
const sceneChangeResult = await detectSceneChange('path/to/current.jpg', 'path/to/previous.jpg');
// or
const sceneChangeResult = await detectSceneChange(currentBuffer, previousBuffer);

console.log(sceneChangeResult);
// {
//   sceneChangeScore: 0-100  // Higher score means more change
// }

// You can also use .then() syntax
detectSabotage(imageBuffer)
  .then((result) => console.log(result))
  .catch((error) => console.error(error));
```

## Score Interpretation

- **defocusScore**: Higher scores (closer to 100) indicate more severe defocus
- **blackoutScore**: Higher scores (closer to 100) indicate more severe blackout
- **flashScore**: Higher scores (closer to 100) indicate more severe flash
- **sceneChangeScore**: Higher scores (closer to 100) indicate more significant scene changes
- **smearScore**: Higher scores (closer to 100) indicate more severe smears or obstructions on the camera lens

## Requirements

- Node.js 14 or higher
- OpenCV 4.x

## Building from Source

```bash
npm install
npm run rebuild
```

## License

MIT
