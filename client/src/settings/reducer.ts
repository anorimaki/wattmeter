import { createReducer } from "typesafe-actions";
import actions from "./actions"
import { Settings } from "settings";

export interface State {
    value?: Settings
}

const reducer = createReducer<State>({})
    .handleAction( actions.set, (_, action) => { return { value: action.payload } } )

export default reducer