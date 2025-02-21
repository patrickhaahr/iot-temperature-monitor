// Service Menu Controls
class ServiceMenu {
    constructor() {
        this.sidebar = document.getElementById('serviceModal');
        this.openButton = document.getElementById('openServiceMenu');
        this.closeButton = document.getElementById('closeServiceMenu');
        this.settingsForm = document.getElementById('settingsForm');
        this.exportButton = document.getElementById('exportData');
        this.resetWifiButton = document.getElementById('resetWifi');
        this.isLargeScreen = window.innerWidth >= 1024; // lg breakpoint is 1024px

        this.initializeEventListeners();
        this.loadSettings(); // Load settings immediately
        
        // Initialize button visibility
        this.updateButtonVisibility();
    }

    initializeEventListeners() {
        this.openButton.addEventListener('click', () => this.show());
        this.closeButton.addEventListener('click', () => this.hide());
        this.settingsForm.addEventListener('submit', (e) => this.handleSettingsSave(e));
        this.exportButton.addEventListener('click', () => this.handleDataExport());
        this.resetWifiButton.addEventListener('click', () => this.handleWifiReset());

        // Handle screen size changes
        window.addEventListener('resize', () => {
            const wasLargeScreen = this.isLargeScreen;
            this.isLargeScreen = window.innerWidth >= 1024;
            
            // If switching between large and smaller screens
            if (wasLargeScreen !== this.isLargeScreen) {
                if (this.isLargeScreen) {
                    // Switching to large screen: show sidebar
                    this.show();
                } else {
                    // Switching to smaller screen: hide sidebar
                    this.hide();
                }
            }
        });
    }

    updateButtonVisibility() {
        // Show/hide the menu button based on sidebar visibility
        if (this.sidebar.classList.contains('translate-x-full')) {
            this.openButton.classList.remove('hidden');
        } else {
            this.openButton.classList.add('hidden');
        }

        // Hide close button on large screens
        if (this.isLargeScreen) {
            this.closeButton.classList.add('hidden');
        } else {
            this.closeButton.classList.remove('hidden');
        }
    }

    show() {
        this.sidebar.classList.remove('translate-x-full');
        this.updateButtonVisibility();
        this.loadSettings();
    }

    hide() {
        this.sidebar.classList.add('translate-x-full');
        this.updateButtonVisibility();
    }

    async loadSettings() {
        try {
            const response = await fetch('/api/system/settings');
            const settings = await response.json();
            
            document.querySelector('[name="tempUpdateInterval"]').value = settings.tempUpdateInterval;
            document.querySelector('[name="loggingInterval"]').value = settings.loggingInterval;
            document.querySelector('[name="maxLogEntries"]').value = settings.maxLogEntries;
        } catch (error) {
            showStatus('Failed to load settings', 'error');
        }
    }

    async handleSettingsSave(e) {
        e.preventDefault();
        
        const settings = {
            tempUpdateInterval: parseInt(document.querySelector('[name="tempUpdateInterval"]').value),
            loggingInterval: parseInt(document.querySelector('[name="loggingInterval"]').value),
            maxLogEntries: parseInt(document.querySelector('[name="maxLogEntries"]').value)
        };

        // Validate settings
        if (!this.validateSettings(settings)) {
            return;
        }

        try {
            const response = await fetch('/api/system/settings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Accept': 'application/json'
                },
                body: JSON.stringify(settings)
            });

            const result = await response.json();
            
            if (response.ok) {
                showStatus('Settings saved successfully');
            } else {
                showStatus(result.message || 'Failed to save settings', 'error');
            }
        } catch (error) {
            console.error('Error saving settings:', error);
            showStatus('Failed to save settings: Network error', 'error');
        }
    }

    validateSettings(settings) {
        if (settings.tempUpdateInterval < 1 || settings.tempUpdateInterval > 60) {
            showStatus('Temperature update interval must be between 1-60 seconds', 'error');
            return false;
        }
        if (settings.loggingInterval < 5 || settings.loggingInterval > 3600) {
            showStatus('Logging interval must be between 5-3600 seconds', 'error');
            return false;
        }
        if (settings.maxLogEntries < 100 || settings.maxLogEntries > 10000) {
            showStatus('Max log entries must be between 100-10000', 'error');
            return false;
        }
        return true;
    }

    async handleDataExport() {
        try {
            const response = await fetch('/api/data/export');
            if (response.ok) {
                const blob = await response.blob();
                const url = window.URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = 'temperature_data.json';
                document.body.appendChild(a);
                a.click();
                window.URL.revokeObjectURL(url);
                showStatus('Data exported successfully');
            } else {
                showStatus('Failed to export data', 'error');
            }
        } catch (error) {
            showStatus('Failed to export data', 'error');
        }
    }

    async handleWifiReset() {
        if (confirm('Are you sure you want to reset the WiFi configuration? The device will restart in AP mode.')) {
            try {
                const response = await fetch('/api/system/reset', {
                    method: 'POST'
                });
                
                if (response.ok) {
                    showStatus('WiFi configuration reset. Device will restart...');
                    setTimeout(() => {
                        window.location.reload();
                    }, 3000);
                } else {
                    showStatus('Failed to reset WiFi configuration', 'error');
                }
            } catch (error) {
                showStatus('Failed to reset WiFi configuration', 'error');
            }
        }
    }
} 