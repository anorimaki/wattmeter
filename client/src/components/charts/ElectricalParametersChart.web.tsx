import React from "react";
import { LineChart, Line, XAxis,
     Tooltip, 
     ResponsiveContainer,
     Brush} from 'recharts';


export interface ElectricalParameters {
    voltage: number
    current: number
}

export interface ElectricalParametersProps {
    width?: string | number;
    height?: string | number;
    data: ElectricalParameters[]
}


export class ElectricalParametersChart extends React.Component<ElectricalParametersProps> {
    render() {
        const data = this.props.data.map( (item, index) => {
            return { 
                i: index,
                voltage: item.voltage,
                current: item.current
            }
        })
        return (
            <ResponsiveContainer width={this.props.width} height={this.props.height}>
                <LineChart
                    width={400}
                    height={400}
                    data={data}
                    margin={{ top: 5, right: 20, left: 10, bottom: 5 }}
                >
                    <XAxis dataKey="i" />
                    <Tooltip />
                    <Line type="linear" 
                        dot={false} 
                        isAnimationActive={false}
                        dataKey="voltage" 
                        stroke="#ff7300" 
                        yAxisId={0} />
                    <Line type="linear" 
                        dot={false} 
                        isAnimationActive={false}
                        dataKey="current" 
                        stroke="#73ff00" 
                        yAxisId={1} />
                    {this.brush()}
                </LineChart>
            </ResponsiveContainer>
        );
    }

    //     <Line type="linear" dataKey="current" stroke="#387908" yAxisId={1} />

    private brush() {
        return <Brush dataKey='i' height={30} stroke="#8884d8"/>
    }
}
