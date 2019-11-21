import { SamplesPackage } from "esp32/SamplesPackage"


export type ConnectionHandler = (connected: boolean) => void
export type ReceiveHandler = (samples: SamplesPackage[]) => void
export type ErrorHandler = (error: Error) => void

export class Esp32ConnectionError extends Error {
    constructor(message?: string) {
        super(message); 
        this.name = "Esp32 Connection Error"
        Object.setPrototypeOf(this, new.target.prototype); // restore prototype chain
    }
}


const SAMPLES_SIZE = 1024 / 16;
const ENCODED_SAMPLES_SIZE = SAMPLES_SIZE * 4;
const ENCODED_SAMPLES_PACKAGE_SIZE = 8 + 4 + 4 + ENCODED_SAMPLES_SIZE;

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
            let data = new DataView(message);
            let samplesPackage: SamplesPackage[] = [];

            let offset = 0
            for( var i=0; i<3; ++i ) {
                samplesPackage.push( this.decodePackage( data, offset ) )
                offset += ENCODED_SAMPLES_PACKAGE_SIZE;
            }
            this.onReceive( samplesPackage )
        }
        catch(error) {
            this.onError( error )
        }
    }

    private decodePackage( data: DataView, offset: number ): SamplesPackage {
        let time = data.getBigUint64(offset, true);
        let voltageScaleFactor = data.getFloat32(offset+8, true);
        let currentScaleFactor = data.getFloat32(offset+12, true);

        let samples = []
        offset += (8 + 4 + 4);
        for ( var i=0; i<SAMPLES_SIZE; ++i ) {
            samples.push({
                voltage: data.getInt16(offset, true),
                current: data.getInt16(offset+2, true)
            });
            offset += 4;
        }
        
        return {
            time: Number(time),
            voltageScaleFactor,
            currentScaleFactor,
            samples
        }
    }
}

