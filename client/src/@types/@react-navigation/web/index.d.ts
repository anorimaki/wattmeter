declare module '@react-navigation/web' {

export function createBrowserApp(
    Component: NavigationNavigator<any, any, any>
    ): NavigationContainer;

}