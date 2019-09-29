/**
 * Metro configuration for React Native
 * https://github.com/facebook/react-native
 *
 * @format
 */

const blacklist = require('metro-config/src/defaults/blacklist');

/*
Metro.Logger.on('log', logEntry => {
  if (
    logEntry.action_name === 'Transforming file' &&
    logEntry.action_phase === 'end'
  ) {
    console.log(logEntry.file_name, logEntry.duration_ms);
  }
});*/

module.exports = {
    transformer: {
        getTransformOptions: async () => ({
            transform: {
                experimentalImportSupport: false,
                inlineRequires: false,
            },
        }),
    },
    resolver: {
        blacklistRE: blacklist([])
    }
};
