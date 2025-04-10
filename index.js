const { Worker, isMainThread, parentPort } = require('worker_threads');
const { detectSabotage, detectSceneChange } = require('./build/Release/camera_sabotage_detector.node');

if (!isMainThread) {
  // Worker thread code
  parentPort.on('message', async (message) => {
    try {
      let result;
      if (message.type === 'sabotage') {
        result = detectSabotage(message.input);
      } else if (message.type === 'sceneChange') {
        result = detectSceneChange(message.current, message.previous);
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
 *   - blurScore {number} - 0-100 score indicating blur level
 *   - blackoutScore {number} - 0-100 score indicating blackout level
 *   - flashScore {number} - 0-100 score indicating flash level
 */
async function detectImageSabotage(input) {
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
async function detectImageSceneChange(current, previous) {
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
  detectSabotage: detectImageSabotage,
  detectSceneChange: detectImageSceneChange,
};
