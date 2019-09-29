import { createStackNavigator } from "@react-navigation/stack";
import HomeScreen from 'screens/home/HomeScreen'

const AppNavigator = createStackNavigator({
    Home: {
        screen: HomeScreen
    }
});

export default AppNavigator;