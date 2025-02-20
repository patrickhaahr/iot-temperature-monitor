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
        animation: false, 
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
        x: new Date(new Date().toDateString() + ' ' + item.timestamp),
        y: item.temp
    }));

    tempChart.data.datasets[0].data = chartData;
    tempChart.update('none');
    
    // Debug logging
    console.log('Chart data updated:', chartData);
}

function updateDisplays(temperature, timestamp) {
    document.getElementById('temperature').textContent = temperature.toFixed(1);
    document.getElementById('last-update').textContent = `Last update: ${timestamp}`;
}

function addTemperatureReading(temperature, timestamp) {
    if (isNaN(temperature) || temperature === null) {
        console.error('Invalid temperature reading:', temperature);
        return;
    }

    const reading = {
        timestamp: timestamp,  // Use the timestamp string directly
        temp: parseFloat(temperature)
    };

    // Add to history while maintaining maxDataPoints limit
    temperatureHistory.push(reading);
    if (temperatureHistory.length > maxDataPoints) {
        temperatureHistory.shift();
    }

    updateDisplays(reading.temp, reading.timestamp);
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

// Load historical data when page loads
async function loadHistoricalData() {
    try {
        const response = await fetch('/api/temperature/history');
        if (!response.ok) {
            throw new Error('Failed to load historical data');
        }
        
        const data = await response.json();
        if (data.readings && Array.isArray(data.readings)) {
            // Convert the last maxDataPoints readings into our format
            temperatureHistory = data.readings
                .slice(-maxDataPoints)
                .map(reading => ({
                    timestamp: reading.timestamp,  // Keep the original timestamp string
                    temp: parseFloat(reading.temperature)
                }));
            
            // Update the display with historical data
            if (temperatureHistory.length > 0) {
                const latest = temperatureHistory[temperatureHistory.length - 1];
                updateDisplays(latest.temp, latest.timestamp);
            }
            
            // Update statistics and chart
            updateStatistics();
            updateChart();
        }
    } catch (error) {
        console.warn('Could not load historical data:', error);
    }
}

// Initialize WebSocket connection and load historical data
function initializeMonitoring() {
    loadHistoricalData().then(() => {
        setupWebSocket();
    });
}

// Update WebSocket message handler
function handleWebSocketMessage(event) {
    try {
        const data = JSON.parse(event.data);
        if (data.temperature !== undefined && data.timestamp !== undefined) {
            addTemperatureReading(data.temperature, data.timestamp);
        } else {
            console.warn('Invalid WebSocket message format:', data);
        }
    } catch (error) {
        console.error('Error processing WebSocket message:', error);
    }
}

// Call initialize when page loads
document.addEventListener('DOMContentLoaded', initializeMonitoring);

// Function to fetch and update data from JSON file
async function updateFromJSON() {
    try {
        const response = await fetch('/api/temperature/history');
        if (!response.ok) {
            throw new Error('Failed to load temperature data');
        }
        
        const data = await response.json();
        if (data.readings && Array.isArray(data.readings)) {
            // Get the most recent readings up to maxDataPoints
            temperatureHistory = data.readings
                .slice(-maxDataPoints)
                .map(reading => ({
                    timestamp: reading.timestamp,
                    temp: parseFloat(reading.temperature)
                }));
            
            // Update displays with latest reading
            if (temperatureHistory.length > 0) {
                const latest = temperatureHistory[temperatureHistory.length - 1];
                document.getElementById('temperature').textContent = latest.temp.toFixed(1);
                document.getElementById('last-update').textContent = `Last update: ${latest.timestamp}`;
            }
            
            updateStatistics();
            updateChart();
        }
    } catch (error) {
        console.warn('Could not load temperature data:', error);
    }
}

// WebSocket setup with immediate updates
function setupWebSocket() {
    const ws = new WebSocket(`ws://${window.location.host}/ws`);
    
    ws.onmessage = async (event) => {
        try {
            const data = JSON.parse(event.data);
            if (data.update && data.temperature !== undefined && data.timestamp !== undefined) {
                // Immediately update display with new reading
                document.getElementById('temperature').textContent = 
                    parseFloat(data.temperature).toFixed(1);
                document.getElementById('last-update').textContent = 
                    `Last update: ${data.timestamp}`;
                
                // Fetch complete history for chart and stats
                await updateFromJSON();
            }
        } catch (error) {
            console.error('Error processing WebSocket message:', error);
        }
    };

    ws.onclose = () => {
        console.log('WebSocket connection closed. Reconnecting...');
        setTimeout(setupWebSocket, 2000);
    };

    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
    };
}

// Initialize monitoring
async function initializeMonitoring() {
    await updateFromJSON();  // Load initial data
    setupWebSocket();        // Set up WebSocket for update notifications
}

document.addEventListener('DOMContentLoaded', initializeMonitoring);
