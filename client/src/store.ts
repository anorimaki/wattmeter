import { combineReducers, createStore } from 'redux'
import settingsReducer, { State as SettingsState } from 'settings/reducer'

export interface RootState {
    settings: SettingsState
}

const rootReducer = combineReducers({
    settings: settingsReducer
})

const store = createStore(rootReducer)

export default store