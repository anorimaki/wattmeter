import React, { Component } from 'react';
import { StyleSheet, View, Switch, Text } from 'react-native';
import { ElectricalParametersChart } from 'components/charts/ElectricalParametersChart';
import { WithEsp32ServiceProps, withEsp32Service } from 'esp32/ESp32ServiceContext';
import { SamplesPackage, Sample } from 'esp32/SamplesPackage';
import { ErrorDialog } from 'errors/ErrorDialog';


const styles = StyleSheet.create({
    root: {
        flex: 1,
        backgroundColor: '#fff'
    },
    bottomPanel: {
        height: "20%",
        flexDirection: 'row',
    },
    controlsPanel: {
        flex: 1,
        flexDirection: 'column'
    },
    infoPanel: {
        flex: 1,
    },
    switchStyle: {}
});


type ComponentProps = WithEsp32ServiceProps

interface Sate {
    connected: boolean
    samples: Sample[]
}

const MAX_SAMPLES_SHOWN = 1024;

class HomeScreen extends Component<ComponentProps, Sate> {
    constructor(props: ComponentProps) {
        super(props)

        this.state = {
            connected: false,
            samples: []
        }
    }

    componentDidMount() {
        this.connectionAction(true)
    }

    render() {
        return (
            <View style={styles.root}>
                <ElectricalParametersChart width="98%" height="80%" data={this.state.samples} />
                <View style={styles.bottomPanel}>
                    {this.controlPanel()}
                    <View style={styles.infoPanel}></View>
                </View>
            </View>
        );
    }

    private controlPanel() {
        return (
            <View style={styles.controlsPanel}>
                <View style={{flex: 1, flexDirection: 'row'}}>
                    <Text>Connected</Text>
                    <Switch style={styles.switchStyle}
                            value={this.state.connected}
                            onValueChange={ value => { this.connectionAction(value) }}/>
                </View>
            </View>
        )
    }

    private connectionAction( connect: boolean ) {
        if ( connect ) {
            this.props.esp32.connect( 
                (connected) => this.onConnectionChanged(connected),
                (samples) => this.onReceive(samples),
                (error) => {
                    ErrorDialog.showError(error)
                    this.props.esp32.close()
                })
        }
        else {
            this.props.esp32.close();
        }
    }

    private onConnectionChanged( connected: boolean ) {
        this.setState( (prevState, _) => {
            return {
                ...prevState,
                connected
            }
        })
    }

    private onReceive( samplesPackages: SamplesPackage[] ) {
        this.setState( (prevState, _) => {
            let samples: Sample[] = []
            for( let i = 0; i<samplesPackages.length; ++i ) {
                let samplesPackage = samplesPackages[i]

                let scaledSamples = samplesPackage.samples
                    .map( sample => { 
                        return {
                            voltage: sample.voltage * samplesPackage.voltageScaleFactor,
                            current: sample.current * samplesPackage.currentScaleFactor
                        };
                    });
                samples.concat( scaledSamples )
            }
                
            let toRemove = (samples.length + prevState.samples.length) - MAX_SAMPLES_SHOWN 
            if ( toRemove < 0 ) {
                toRemove = 0
            }
            let newSamples = prevState.samples.slice( toRemove )
            newSamples.push.apply( newSamples, samples )
            return {
                ...prevState,
                samples: newSamples
            }
        })
    }
}


export default withEsp32Service(HomeScreen)