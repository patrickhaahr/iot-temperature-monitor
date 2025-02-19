let ws = null;
let reconnectAttempts = 0;
const maxReconnectAttempts = 5;
const reconnectDelay = 5000;

// Temperature history management
const maxDataPoints = 50;  // Maximum number of points to show on the graph
let temperatureHistory = [];

// Initialize the chart
const ctx = document.getElementById('tempChart').getContext('2d');
const tempChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Temperature °C',
            data: [],
            borderColor: 'rgb(59, 130, 246)',
            tension: 0.1,
            fill: false
        }]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
            y: {
                beginAtZero: false,
                title: {
                    display: true,
                    text: 'Temperature (°C)'
                }
            },
            x: {
                title: {
                    display: true,
                    text: 'Time'
                }
            }
        },
        plugins: {
            legend: {
                display: false
            }
        }
    }
});

function updateStatistics() {
    if (temperatureHistory.length === 0) return;

    const temperatures = temperatureHistory.map(item => item.temp);
    const minTemp = Math.min(...temperatures);
    const maxTemp = Math.max(...temperatures);
    const avgTemp = temperatures.reduce((a, b) => a + b) / temperatures.length;

    document.getElementById('min-temp').textContent = minTemp.toFixed(1);
    document.getElementById('max-temp').textContent = maxTemp.toFixed(1);
    document.getElementById('avg-temp').textContent = avgTemp.toFixed(1);
    document.getElementById('sample-count').textContent = temperatures.length;
}

function updateChart() {
    const labels = temperatureHistory.map(item => item.time);
    const data = temperatureHistory.map(item => item.temp);

    tempChart.data.labels = labels;
    tempChart.data.datasets[0].data = data;
    tempChart.update();
}

function addTemperatureReading(temperature) {
    const now = new Date();
    const timeStr = now.toLocaleTimeString();
    
    temperatureHistory.push({
        time: timeStr,
        temp: temperature
    });

    // Keep only the last maxDataPoints readings
    if (temperatureHistory.length > maxDataPoints) {
        temperatureHistory.shift();
    }

    // Update the display
    document.getElementById('temperature').textContent = temperature.toFixed(1);
    document.getElementById('last-update').textContent = `Last update: ${timeStr}`;
    
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
            if (data.type === 'temperature') {
                addTemperatureReading(data.value);
            }
        } catch (e) {
            console.error('Error parsing WebSocket message:', e);
        }
    };
}

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
