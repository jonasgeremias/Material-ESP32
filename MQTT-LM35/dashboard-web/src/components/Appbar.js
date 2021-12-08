import React from 'react'
import MuiAppBar from '@material-ui/core/AppBar'
import Typography from '@material-ui/core/Typography'
import Toolbar from '@material-ui/core/Toolbar'
import useStyles from './styles'

const Appbar = () => {
    const classes = useStyles()

    return (
        <MuiAppBar position="static" color='inherit' className={ classes.appBar } elevation={ 0 }>
            <Toolbar>
                <Typography component="h1" variant="h6" color="textPrimary" noWrap className={ classes.appBarTitle }>
                    Sistema de monitoramento
                </Typography>
            </Toolbar>
        </MuiAppBar>
    )
}

export default Appbar
