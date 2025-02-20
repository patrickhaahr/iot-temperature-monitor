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
    // Reset stats if no data
    if (temperatureHistory.length === 0) {
        document.getElementById('min-temp').textContent = '--.-';
        document.getElementById('max-temp').textContent = '--.-';
        document.getElementById('avg-temp').textContent = '--.-';
        document.getElementById('sample-count').textContent = '0';
        return;
    }

    const temperatures = temperatureHistory.map(item => item.temp);
    const min = Math.min(...temperatures);
    const max = Math.max(...temperatures);
    const avg = temperatures.reduce((a, b) => a + b) / temperatures.length;

    document.getElementById('min-temp').textContent = min.toFixed(1);
    document.getElementById('max-temp').textContent = max.toFixed(1);
    document.getElementById('avg-temp').textContent = avg.toFixed(1);
    document.getElementById('sample-count').textContent = temperatureHistory.length.toString();
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

// Single WebSocket initialization function
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
        console.log('WebSocket connection closed');
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
    
    ws.onmessage = async (event) => {
        try {
            const data = JSON.parse(event.data);
            console.log('WebSocket message received:', data);
            
            if (data.update && data.temperature !== undefined && data.timestamp !== undefined) {
                // Update display immediately
                document.getElementById('temperature').textContent = 
                    parseFloat(data.temperature).toFixed(1);
                document.getElementById('last-update').textContent = 
                    `Last update: ${data.timestamp}`;
                
                // Small delay to ensure the JSON file is updated
                await new Promise(resolve => setTimeout(resolve, 100));
                
                // Update from JSON file
                await updateFromJSON();
            }
        } catch (error) {
            console.error('Error processing WebSocket message:', error);
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

// Initialize monitoring with latest temperature
async function initializeMonitoring() {
    try {
        // Load historical data first
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
                    timestamp: reading.timestamp,
                    temp: parseFloat(reading.temperature)
                }));
            
            // Update displays with the latest reading immediately
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
    
    // Initialize WebSocket after loading initial data
    initWebSocket();
}

// Remove setupWebSocket function and only use initWebSocket
// Check if in AP mode and initialize accordingly
if (window.location.hostname.includes('ESP32_Config')) {
    document.getElementById('wifi-config').classList.remove('hidden');
    document.getElementById('temperature-display').classList.add('hidden');
} else {
    // Initialize monitoring when page loads
    document.addEventListener('DOMContentLoaded', initializeMonitoring);
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

// Update from JSON without adding duplicate entries
async function updateFromJSON() {
    try {
        const response = await fetch('/api/temperature/history');
        if (!response.ok) {
            throw new Error('Failed to load temperature data');
        }
        
        const data = await response.json();
        if (data.readings && Array.isArray(data.readings)) {
            // Get the most recent readings up to maxDataPoints
            const newHistory = data.readings
                .slice(-maxDataPoints)
                .map(reading => ({
                    timestamp: reading.timestamp,
                    temp: parseFloat(reading.temperature)
                }));
            
            // Only update if the data has actually changed
            if (JSON.stringify(newHistory) !== JSON.stringify(temperatureHistory)) {
                temperatureHistory = newHistory;
                updateStatistics();
                updateChart();
            }
        }
    } catch (error) {
        console.warn('Could not load temperature data:', error);
    }
}
