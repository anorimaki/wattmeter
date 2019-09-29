export interface Sample {
    voltage: number
    current: number
}

export type ConnectionHandler = (connected: boolean) => void
export type ReceiveHandler = (samples: Sample[]) => void
export type ErrorHandler = (error: Error) => void

export class Esp32ConnectionError extends Error {
    constructor(message?: string) {
        super(message); 
        this.name = "Esp32 Connection Error"
        Object.setPrototypeOf(this, new.target.prototype); // restore prototype chain
    }
}

export class Esp32Service {
    private socket?: WebSocket
    private onReceive: ReceiveHandler
    private onError: ErrorHandler
    private url: string

    constructor( url: string ) {
        this.url = url
        this.onReceive = () => {}
        this.onError = () => {}
    }

    connect( connectionHandle: ConnectionHandler,
            receiveHandler: ReceiveHandler,
            errorHandler: ErrorHandler ) {
        this.onReceive = receiveHandler
        this.onError = errorHandler

        var _this = this;
        this.socket = new WebSocket(this.url);
        this.socket.binaryType = 'arraybuffer';
        this.socket.onopen = () => {
            connectionHandle(true)
        }
        this.socket.onerror = function(ev) { _this.errorHandler( this, ev ) }
        this.socket.onmessage = function(ev) { _this.receiveHandler( this, ev ) }
        this.socket.onclose = () => {
            connectionHandle(false)
        }
    }

    close() {
        if ( this.socket === undefined ) {
            return;
        }
        this.socket.close()
        this.socket = undefined
    }

    private errorHandler( socket: WebSocket, ev: Event ) {
        let msg = "Unknown connection error"
        switch(socket.readyState) {
            case WebSocket.CLOSED:
                msg = `Connecting to ${this.url}`
                break;
        }
        let error = new Esp32ConnectionError(msg)
        this.onError( error )
    }

    private receiveHandler( _: WebSocket, ev: MessageEvent ) {
        try {
            let message = ev.data;
            let values = new DataView(message);
            let samples = []
            for( var i=0; i<values.byteLength; i=i+8 ) {
                samples.push({
                    voltage: values.getFloat32(i, true),
                    current: values.getFloat32(i+4, true)
                })
            }

         /*   var values = new Int16Array(message);
            let samples = values.reduce( (result, value, index, values) => {
                    if ( (index % 2) === 0 ) {
                        result.push( {
                            voltage: value,
                            current: values[index+1]
                        } )
                    }
                    return result
                }, new Array<Sample>() ) */
/*            
            let messageObj = JSON.parse( message )
            let samples = messageObj.map( (item: any) => {
                if ( (item.v === undefined) || (item.c === undefined) ) {
                    throw new Esp32ConnectionError("Invalid data received")
                }
                return {
                    voltage: item.v,
                    current: item.c
                }
            });
*/
            this.onReceive( samples )
        }
        catch(error) {
            this.onError( error )
        }
    }
}

