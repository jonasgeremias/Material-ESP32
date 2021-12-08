import React, { useState } from 'react'
import GoogleMapReact from 'google-map-react'
import useStyles from './styles'
import Typography from '@material-ui/core/Typography'
import Slider from '@material-ui/core/Slider'

const Card = ({ data }) => {
    const classes = useStyles()

    return (
        <>
            <div lat={ data.lat } lng={ data.long } className={ classes.mapConnector } />
            <div className={ classes.mapCard }>
                <Typography variant='body2' gutterBottom>Temp { data.temp }ºC</Typography>
                <Typography variant='caption' gutterBottom>{ data.description }</Typography>
                <Typography variant='caption' style={{ opacity: .7 }}>Lat { data.lat.slice(0, 7) } | Lng { data.long.slice(0, 7) }</Typography>
                <Typography variant='caption' style={{ opacity: .7 }}>Período { data.period } | ID { data.id }</Typography>
            </div>
        </>
    )
}

const Map = ({ users }) => {
    const classes = useStyles()
    const [radius, setRadius] = useState(50)
    const data = Object.values(users)

    const handleChangeRadius = (_, newValue) => {
        setRadius(newValue)
    }

    return (
        <div>
            <Typography variant='h6' gutterBottom>Mapa de calor</Typography>
            <div className={ classes.map }>
                <GoogleMapReact bootstrapURLKeys={{ key: "AIzaSyDtjMmiZi7mJjnKIiXfQ26BibTHg2xx7aM", libraries: ['visualization'] }} defaultCenter={{ lat: -28.693595, lng: -49.422211 }}
                    defaultZoom={ 10 } heatmap={{ positions: data.map((el, i) => ({ ...el, weight: el.temp, lng: el.long })), options: { radius } }}
                >
                    { data.map((el, i) => <Card key={ i.toString() } lat={ el.lat } lng={ el.long } data={ el } />) }
                </GoogleMapReact>
            </div>
            <Typography>Raio do mapa de calor</Typography>
            <Slider defaultValue={ 50 } step={ 10 } min={ 10 } max={ 200 } value={ radius } onChange={ handleChangeRadius }/>
        </div>
    )
}

export default Map
