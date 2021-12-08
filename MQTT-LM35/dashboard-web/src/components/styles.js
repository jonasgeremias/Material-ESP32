import { makeStyles } from '@material-ui/core/styles'

export default makeStyles((theme) => ({
    appBar: {
        backgroundColor: '#121212',
        zIndex: 0,
    },
    appBarTitle: {
        flex: 1,
        paddingLeft: 30
    },
    map: {
        marginBottom: 20,
        width: '100%',
        height: '70vh',
        borderRadius: 10,
        overflow: 'hidden'
    },
    mapCard: {
        position: 'absolute',
        left: -85,
        top: 30,
        backgroundColor: theme.palette.background.paper,
        borderRadius: 8,
        padding: 8,
        paddingBottom: 4,
        width: 170,
        display: 'flex',
        flexDirection: 'column'
    },
    mapConnector: {
        width: 2,
        height: 30,
        backgroundColor: theme.palette.background.paper
    },
    historic: {
        marginTop: 20,
        marginBottom: 50
    }
}))