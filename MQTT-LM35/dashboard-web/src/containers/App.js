import React, { useEffect, useState } from 'react'
import useStyles from './styles'
import { Appbar, Historic, Map } from 'components'
import mqtt from 'mqtt'

function App() {
    const classes = useStyles()
    const [historic, setHistoric] = useState([])
    const [users, setUsers] = useState({})

    useEffect(() => {
        const client = mqtt.connect('wss://e066f399.us-east-1.emqx.cloud:8084/mqtt', {
            clientId: 'clientFront123', protocol: 'wss', port: 8084, username: 'clientFront123', password: 'clientFront123'
        })

        client.on('connect', function () {
            console.log('Client connected')
            client.subscribe('device/+/realtime', function (err) {
                if (!err) {
                    console.log('Subscribed')
                }
            })
        })

        client.on('error', function(error) {
            console.log('Connect error: ', error)
        })
          
        client.on('message', function (topic, message) {
            const id = topic.split('/')[1]
            const data = JSON.parse(message.toString())

            if (data.id >= 0 && data.id < 4) {
                setUsers(prev => ({ ...prev, [String(id)]: data }))
                setHistoric(prev => [data, ...prev.slice(0, 20)])
            }
        })
        
        return () => {
            client.end()
        }
    }, [])

    return (
        <>
            <Appbar/>
            <div className={ classes.container }>
                <Map users={ users }/>
                <Historic data={ historic }/>
            </div>
        </>
    )
}

export default App