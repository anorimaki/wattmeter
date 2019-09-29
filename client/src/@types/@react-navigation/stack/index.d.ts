declare module '@react-navigation/stack' {

import { NavigationContainer,
    NavigationRouteConfigMap,
    HeaderMode,
    NavigationTransitionProps,
    TransitionConfig,
    NavigationPathsConfig,
    NavigationParams,
    NavigationScreenConfig,
    NavigationScreenOptions } from '@react-navigation/core'

import { StyleProp, ViewStyle } from 'react-native';

export interface NavigationStackViewConfig {
    mode?: 'card' | 'modal';
    headerMode?: HeaderMode;
    headerBackTitleVisible?: boolean;
    headerTransitionPreset?: 'fade-in-place' | 'uikit';
    headerLayoutPreset?: 'left' | 'center';
    cardShadowEnabled?: boolean;
    cardOverlayEnabled?: boolean;
    cardStyle?: StyleProp<ViewStyle>;
    transparentCard?: boolean;
    transitionConfig?: (
        transitionProps: NavigationTransitionProps,
        prevTransitionProps: NavigationTransitionProps,
        isModal: boolean
    ) => TransitionConfig;
    onTransitionStart?: (
        transitionProps: NavigationTransitionProps,
        prevTransitionProps?: NavigationTransitionProps
    ) => Promise<void> | void;
    onTransitionEnd?: (
        transitionProps: NavigationTransitionProps,
        prevTransitionProps?: NavigationTransitionProps
    ) => void;
}

export interface NavigationStackRouterConfig {
    headerTransitionPreset?: 'fade-in-place' | 'uikit';
    initialRouteName?: string;
    initialRouteParams?: NavigationParams;
    paths?: NavigationPathsConfig;
    defaultNavigationOptions?: NavigationScreenConfig<NavigationScreenOptions>;
    navigationOptions?: NavigationScreenConfig<NavigationScreenOptions>;
    initialRouteKey?: string;
}

export interface StackNavigatorConfig extends NavigationStackViewConfig,
    NavigationStackRouterConfig {
    containerOptions?: any;
}

export function createStackNavigator(
    routeConfigMap: NavigationRouteConfigMap,
    stackConfig?: StackNavigatorConfig
): NavigationContainer;

}