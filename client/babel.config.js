module.exports = function (api) {
    api.cache(true);
    return {
        presets: [["module:metro-react-native-babel-preset"], ['react-app']],
        plugins: [
            ["module-resolver", {
                "root": ["./src"],
                "extensions": [".ts", ".ios.ts", ".android.ts", ".web.ts", ".native.ts",
                                ".tsx", ".ios.tsx", ".android.tsx", ".web.tsx", ".native.tsx"],
                "alias": {
                    "components": "./src/components",
                    "routes": "./src/routes",
                    "screens": "./src/screens"
                }
               /* "alias": {
                    "^react-native$": "react-native-web"
                } */
            }]
        ]
    };
};