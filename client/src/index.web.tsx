import {AppRegistry} from 'react-native';
import App from './App';

AppRegistry.registerComponent('Watmetter', () => App);

AppRegistry.runApplication('Watmetter', {
    initialProps: {},
    rootTag: document.getElementById('react-root')
});