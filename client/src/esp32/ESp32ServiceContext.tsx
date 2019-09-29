import React from "react";
import { Esp32Service } from "./Esp32Service";

const Context = React.createContext<Esp32Service|undefined>(undefined);

export const Esp32ServiceProvider = Context.Provider

export interface WithEsp32ServiceProps {
    esp32: Esp32Service
}

export function withEsp32Service<T>( Component: React.ComponentType<T> ): React.ComponentType<T> {
    const Wrapper = (props: T) => (
            <Context.Consumer>
                { (esp32) => (
                    <Component esp32={esp32} {...props}/>
                )}
            </Context.Consumer>
        );
    return Wrapper;
}