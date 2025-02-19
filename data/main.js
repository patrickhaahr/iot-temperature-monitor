let ws = null;
let reconnectAttempts = 0;
const maxReconnectAttempts = 5;
const reconnectDelay = 5000;

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
                document.getElementById('temperature').textContent = 
                    data.value.toFixed(1);
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
