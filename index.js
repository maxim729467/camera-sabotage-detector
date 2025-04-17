const { Worker, isMainThread, parentPort } = require('worker_threads');

if (!isMainThread) {
  // Worker thread code
  const { detectSabotage: nativeDetectSabotage, detectSceneChange: nativeDetectSceneChange } = require('./build/Release/camera_sabotage_detector');

  parentPort.on('message', async (message) => {
    try {
      let result;
      if (message.type === 'sabotage') {
        result = nativeDetectSabotage(message.input);
      } else if (message.type === 'sceneChange') {
        result = nativeDetectSceneChange(message.current, message.previous);
      }
      parentPort.postMessage({ success: true, result });
    } catch (error) {
      parentPort.postMessage({ success: false, error: error.message });
    }
  });
  return;
}

/**
 * Detects various types of camera sabotage in an image.
 * @param {string|Buffer} input - Image file path or buffer containing image data
 * @returns {Promise<Object>} Promise resolving to an object containing:
 *   - defocusScore {number} - 0-100 score indicating defocus level
 *   - blackoutScore {number} - 0-100 score indicating blackout level
 *   - flashScore {number} - 0-100 score indicating flash level
 *   - smearScore {number} - 0-100 score indicating smear level
 */
async function detectSabotage(input) {
  return new Promise((resolve, reject) => {
    const worker = new Worker(__filename);

    worker.on('message', (message) => {
      if (message.success) {
        resolve(message.result);
      } else {
        reject(new Error(message.error));
      }
      worker.terminate();
    });

    worker.on('error', (error) => {
      reject(error);
      worker.terminate();
    });

    worker.postMessage({ type: 'sabotage', input });
  });
}

/**
 * Detects significant changes between two consecutive frames.
 * @param {string|Buffer} current - Current frame image path or buffer
 * @param {string|Buffer} previous - Previous frame image path or buffer
 * @returns {Promise<Object>} Promise resolving to an object containing:
 *   - sceneChangeScore {number} - 0-100 score indicating scene change level
 */
async function detectSceneChange(current, previous) {
  return new Promise((resolve, reject) => {
    const worker = new Worker(__filename);

    worker.on('message', (message) => {
      if (message.success) {
        resolve(message.result);
      } else {
        reject(new Error(message.error));
      }
      worker.terminate();
    });

    worker.on('error', (error) => {
      reject(error);
      worker.terminate();
    });

    worker.postMessage({ type: 'sceneChange', current, previous });
  });
}

module.exports = {
  detectSabotage,
  detectSceneChange,
};
