let ws = null;
let reconnectAttempts = 0;
const maxReconnectAttempts = 5;
const reconnectDelay = 5000;

// Temperature history management
const maxDataPoints = 50;  // Maximum number of points to show on the graph
let temperatureHistory = [];

// Initialize the chart with proper configuration
const ctx = document.getElementById('tempChart').getContext('2d');
const tempChart = new Chart(ctx, {
    type: 'line',
    data: {
        datasets: [{
            label: 'Temperature °C',
            data: [],
            borderColor: 'rgb(59, 130, 246)',
            backgroundColor: 'rgba(59, 130, 246, 0.1)',
            tension: 0.3,
            fill: true,
            pointRadius: 3,
            pointHoverRadius: 5
        }]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,  // Disable animations for better performance
        scales: {
            y: {
                beginAtZero: false,
                title: {
                    display: true,
                    text: 'Temperature (°C)'
                }
            },
            x: {
                type: 'time',
                time: {
                    unit: 'second',
                    parser: 'YYYY-MM-DD HH:mm:ss',
                    tooltipFormat: 'HH:mm:ss',
                    displayFormats: {
                        second: 'HH:mm:ss'
                    }
                },
                title: {
                    display: true,
                    text: 'Time'
                }
            }
        },
        plugins: {
            legend: {
                display: false
            },
            tooltip: {
                callbacks: {
                    label: function(context) {
                        return `Temperature: ${context.parsed.y.toFixed(1)}°C`;
                    }
                }
            }
        }
    }
});

function updateStatistics() {
    if (temperatureHistory.length === 0) {
        document.getElementById('min-temp').textContent = '--.-';
        document.getElementById('max-temp').textContent = '--.-';
        document.getElementById('avg-temp').textContent = '--.-';
        document.getElementById('sample-count').textContent = '0';
        return;
    }

    const temperatures = temperatureHistory.map(item => item.temp);
    const minTemp = Math.min(...temperatures);
    const maxTemp = Math.max(...temperatures);
    const avgTemp = temperatures.reduce((a, b) => a + b, 0) / temperatures.length;

    document.getElementById('min-temp').textContent = minTemp.toFixed(1);
    document.getElementById('max-temp').textContent = maxTemp.toFixed(1);
    document.getElementById('avg-temp').textContent = avgTemp.toFixed(1);
    document.getElementById('sample-count').textContent = temperatures.length;
}

function updateChart() {
    const chartData = temperatureHistory.map(item => ({
        x: item.timestamp,  // This should be a Date object
        y: item.temp
    }));

    tempChart.data.datasets[0].data = chartData;
    tempChart.update('none');
    
    // Debug logging
    console.log('Chart data updated:', chartData);
}

function addTemperatureReading(temperature, timestamp) {
    // Validate the temperature reading
    if (isNaN(temperature) || temperature === null) {
        console.error('Invalid temperature reading:', temperature);
        return;
    }

    // Create a new Date object from the timestamp
    const date = new Date(timestamp * 1000); // Convert Unix timestamp to milliseconds
    
    const reading = {
        timestamp: date,  // Store as Date object
        temp: parseFloat(temperature)
    };

    temperatureHistory.push(reading);

    // Keep only the last maxDataPoints readings
    if (temperatureHistory.length > maxDataPoints) {
        temperatureHistory.shift();
    }

    // Update the current temperature display
    document.getElementById('temperature').textContent = reading.temp.toFixed(1);
    document.getElementById('last-update').textContent = 
        `Last update: ${date.toLocaleTimeString()}`;
    
    // Debug logging
    console.log('New reading added:', reading);
    console.log('History length:', temperatureHistory.length);
    
    updateStatistics();
    updateChart();
}

function initWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;
    
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
        console.log('WebSocket connected');
        document.getElementById('connection-status').textContent = 'Connected';
        reconnectAttempts = 0;
        temperatureHistory = []; // Clear history on new connection
    };
    
    ws.onclose = () => {
        console.log('WebSocket disconnected');
        document.getElementById('connection-status').textContent = 'Disconnected';
        
        if (reconnectAttempts < maxReconnectAttempts) {
            reconnectAttempts++;
            document.getElementById('connection-status').textContent = 
                `Reconnecting (${reconnectAttempts}/${maxReconnectAttempts})...`;
            setTimeout(initWebSocket, reconnectDelay);
        }
    };
    
    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
    };
    
    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            console.log('WebSocket message received:', data);  // Debug logging
            
            if (data.temperature !== undefined && data.timestamp !== undefined) {
                addTemperatureReading(data.temperature, data.timestamp);
            } else {
                console.warn('Received invalid message format:', data);
            }
        } catch (e) {
            console.error('Error parsing WebSocket message:', e);
        }
    };
}

// Initialize WebSocket connection when page loads
document.addEventListener('DOMContentLoaded', () => {
    initWebSocket();
});

// Handle WiFi configuration form
document.getElementById('wifi-form')?.addEventListener('submit', async (e) => {
    e.preventDefault();
    
    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;
    
    try {
        const response = await fetch('/api/wifi/configure', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ ssid, password }),
        });
        
        if (response.ok) {
            alert('WiFi configuration saved. Device will restart...');
        } else {
            const data = await response.json();
            alert(`Error: ${data.message || 'Failed to save WiFi configuration'}`);
        }
    } catch (error) {
        alert('Error saving WiFi configuration');
        console.error('Error:', error);
    }
});

// Check if in AP mode
if (window.location.hostname.includes('ESP32_Config')) {
    document.getElementById('wifi-config').classList.remove('hidden');
    document.getElementById('temperature-display').classList.add('hidden');
} else {
    // Initialize WebSocket connection
    initWebSocket();
}
