import React from 'react'
import ReactDOM from 'react-dom'
import './index.css'
import App from './containers/App'
import { createTheme, ThemeProvider } from '@material-ui/core/styles'
import CssBaseline from '@material-ui/core/CssBaseline'

const theme = createTheme({
    palette: {
        type: 'dark',
        primary: {
            main: '#673ab7' 
        },
        background: {
            default: '#121212',
            paper: '#242424'
        },
        text: {
            primary: '#d9d9d9'
        }
    },
})

ReactDOM.render(
    <React.StrictMode>
        <ThemeProvider theme={ theme }>
            <>
                <CssBaseline />
                <App />
            </>
        </ThemeProvider>
    </React.StrictMode>,
    document.getElementById('root')
)