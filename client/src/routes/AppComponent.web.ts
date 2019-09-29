import { createBrowserApp } from "@react-navigation/web";
import AppNavigator from './navigator'

export const AppContainer = createBrowserApp(AppNavigator);
