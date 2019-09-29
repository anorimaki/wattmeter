import { ErrorDialog } from "./ErrorDialog";

export function handleError( e: Error ) {
    console.log(e)
}

export function showErrorOf( title: string, code: () => void ): Error | undefined {
    try {
        code()
        return undefined
    }
    catch( e ) {
        ErrorDialog.show( title, e )
        return e
    }
}