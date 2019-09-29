import { AsyncStorage } from "react-native";

export interface Settings {
    esp32: {
        url: string
    }
}

const defaultSettings = {
    esp32: {
        //url: "ws://192.168.1.46:80/ws"
        url: "ws://192.168.1.46:8080/ws"
    }
}

function deserialize( str: string ): Settings {
    const storedSettings = JSON.parse(str)
    return {
        esp32: {
            url: storedSettings.esp32.url
        }
    }
}

export function loadSettings(): Promise<Settings> {
    return AsyncStorage.getItem('settings')
        .then( storedSettingsStr => 
                storedSettingsStr===null ? defaultSettings : deserialize(storedSettingsStr) )
}