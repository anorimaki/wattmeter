import { createAction } from 'typesafe-actions';
import { Settings } from 'settings';

const actions = {
    set: createAction( 'settings/set', action => {
            return (settings: Settings) => action(settings)
        })
}

export default actions;