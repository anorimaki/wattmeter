import { Alert as NativeAlert, Platform } from "react-native";
import WebAlert from "./alert"

export class ErrorDialog {
    static showError( error: Error ) {
        this.show( error.name, error.message )
    }

    static show( title: string, message: string ) {
        const AlertImpl = (Platform.OS === 'web') ? WebAlert : NativeAlert
        AlertImpl.alert(title, message)
    }
}