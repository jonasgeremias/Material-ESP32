import React from 'react'
import Table from '@material-ui/core/Table'
import TableBody from '@material-ui/core/TableBody'
import TableCell from '@material-ui/core/TableCell'
import TableContainer from '@material-ui/core/TableContainer'
import TableHead from '@material-ui/core/TableHead'
import TableRow from '@material-ui/core/TableRow'
import Paper from '@material-ui/core/Paper'
import { Typography } from '@material-ui/core'
import useStyles from './styles'

const Historic = ({ data }) => {
    const classes = useStyles()

    return (
        <div className={ classes.historic }>
            <Typography variant='h6' gutterBottom>Histórico de leituras</Typography>
            <TableContainer component={ Paper }>
                <Table>
                    <TableHead>
                        <TableRow>
                            <TableCell>ID</TableCell>
                            <TableCell>Descrição</TableCell>
                            <TableCell align="right">Temperatura</TableCell>
                            <TableCell align="right">Lat</TableCell>
                            <TableCell align="right">Lng</TableCell>
                            <TableCell align="right">MAC</TableCell>
                            <TableCell align="right">Período</TableCell>
                        </TableRow>
                    </TableHead>
                    <TableBody>
                        { data.map((row, i) => (
                            <TableRow key={ i }>
                                <TableCell component="th" scope="row">
                                    { row.id }
                                </TableCell>
                                <TableCell>{ row.description }</TableCell>
                                <TableCell align="right">{ row.temp }</TableCell>
                                <TableCell align="right">{ row.lat }</TableCell>
                                <TableCell align="right">{ row.long }</TableCell>
                                <TableCell align="right">{ row.mac }</TableCell>
                                <TableCell align="right">{ row.period }</TableCell>
                            </TableRow>
                        )) }
                    </TableBody>
                </Table>
            </TableContainer>
        </div>
    )
}

export default Historic