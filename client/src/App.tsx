import React from 'react';
import { AppContainer } from 'routes/AppComponent'
import { loadSettings } from 'settings';
import { Provider, connect } from 'react-redux'
import store, {RootState} from 'store';
import settingsActions from 'settings/actions'
import { Esp32Service } from 'esp32/Esp32Service';
import { Esp32ServiceProvider } from 'esp32/ESp32ServiceContext';


type Props = 
    ReturnType<typeof mapStateToProps> &
    typeof mapDispatchToProps

/* 'settings' is in store and not in the AppComponent state because in the future
    we can add a view to change it */
class AppComponent extends React.Component<Props> {
    componentDidMount() {
        loadSettings()
            .then( settings => this.props.setSettings(settings) )
    }

    render() {
        const { settings } = this.props

        if ( settings === undefined ) {
            return null
        }
        return (
            <Esp32ServiceProvider value={new Esp32Service(settings.esp32.url)} >
                <AppContainer />
            </Esp32ServiceProvider>
        );
    }
}

function mapStateToProps( state: RootState ) {
    return {
        settings: state.settings.value
    }
}

const mapDispatchToProps = {
    setSettings: settingsActions.set
}

const ConnectedAppComponent = connect(mapStateToProps, mapDispatchToProps)(AppComponent)


const App = () => {
    return (
        <Provider store={store}>
            <ConnectedAppComponent/>
        </Provider>
    )
}

export default App
