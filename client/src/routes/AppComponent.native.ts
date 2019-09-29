import { createAppContainer } from "react-navigation";
import AppNavigator from './navigator'

export const AppContainer = createAppContainer(AppNavigator as any); 
