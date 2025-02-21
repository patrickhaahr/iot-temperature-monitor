class StatusMessage {
    constructor() {
        this.statusDiv = document.getElementById('statusMessage');
        this.statusText = document.getElementById('statusText');
    }

    show(message, type = 'success') {
        this.statusText.textContent = message;
        
        this.statusDiv.classList.remove('hidden');
        this.statusDiv.firstElementChild.className = type === 'success' 
            ? 'bg-green-100 border border-green-400 text-green-700 px-4 py-3 rounded relative'
            : 'bg-red-100 border border-red-400 text-red-700 px-4 py-3 rounded relative';
        
        setTimeout(() => {
            this.statusDiv.classList.add('hidden');
        }, 3000);
    }
}

// Create global instance
const statusMessage = new StatusMessage();
const showStatus = (message, type) => statusMessage.show(message, type); 