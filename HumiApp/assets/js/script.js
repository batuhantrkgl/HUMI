const devMode = true; // Enable developer mode
const humidityData = [];
const timestamps = [];
const alertHistory = [];
const careTips = [];
const colors = [
    'rgba(75, 192, 192, 1)',
    'rgba(255, 99, 132, 1)',
    'rgba(255, 206, 86, 1)',
    'rgba(54, 162, 235, 1)',
    'rgba(255, 159, 64, 1)',
];

// Mock plant data for demonstration
const plants = [
    { humidity: 70, recommendedHumidity: 60 },
    { humidity: 50, recommendedHumidity: 60 },
    { humidity: 80, recommendedHumidity: 60 },
];

// Submit care tip
function submitTip() {
    const tipInput = document.getElementById('tip-input');
    const tip = tipInput.value.trim();
    if (tip) {
        careTips.push(tip);
        tipInput.value = '';
        updateTipsList();
    }
}

function updateTipsList() {
    const tipsList = document.getElementById('tips-list');
    tipsList.innerHTML = '';
    careTips.forEach(tip => {
        const tipItem = document.createElement('div');
        tipItem.textContent = tip;
        tipsList.appendChild(tipItem);
    });
}

// Render plant data
function renderPlants(data) {
    const plantList = document.getElementById('plant-list');
    plantList.innerHTML = '';

    data.forEach((plant, index) => {
        const plantContainer = document.createElement('div');
        plantContainer.classList.add('team-member-container');

        const humidity = plant.humidity;
        const recommended = plant.recommendedHumidity;
        const alertMessage = checkHumidity(index, humidity, recommended);

        const plantInfo = `
            <div class="team-member-info">
                <p class="teammate-name">Bitki ${index + 1}</p>
                <p>Nem Durumu: <span id="humidity-${index + 1}">${humidity}%</span></p>
                <p>Önerilen Nem: <span>${recommended}%</span></p>
                ${alertMessage ? `<p class="alert">${alertMessage}</p>` : ''}
                <div class="button-container">
                    <button class="action-button" onclick="takePhoto()">
                        <i class="fas fa-camera"></i>
                    </button>
                    <button class="action-button" onclick="waterPlant(${index + 1})">
                        <i class="fas fa-water"></i>
                    </button>
                    <button class="action-button" onclick="toggleFan(${index + 1})">
                        <i class="fas fa-fan"></i>
                    </button>
                </div>
            </div>
        `;
        plantContainer.innerHTML = plantInfo;
        plantList.appendChild(plantContainer);
    });
}

// Check humidity levels
function checkHumidity(index, humidity, recommended) {
    if (humidity < (recommended - 10) || humidity > (recommended + 10)) {
        return `Uyarı: Bitki ${index + 1} nem seviyesi uygun değil! (${humidity}%)`;
    }
    return '';
}

// Update humidity and chart
function updateHumidity() {
    const currentTime = new Date().toLocaleTimeString();
    const plantList = document.getElementById('plant-list');

    if (devMode) {
        // Simulate mock data
        plants.forEach((plant, index) => {
            const humidity = Math.floor(Math.random() * 100);
            plant.humidity = humidity; // Update humidity
            humidityData[index] = humidityData[index] || [];
            humidityData[index].push(humidity);
            checkAndLogAlert(index, humidity);
        });
        timestamps.push(currentTime);
        renderPlants(plants);
        updateChart();
    }

    plantList.scrollTop = plantList.scrollHeight; // Scroll to bottom
}

// Check and log alerts
function checkAndLogAlert(index, humidity) {
    const recommendedHumidity = plants[index].recommendedHumidity;
    if (humidity < (recommendedHumidity - 10) || humidity > (recommendedHumidity + 10)) {
        const alertMessage = `Bitki ${index + 1}: Nem seviyesi ${humidity}%`;
        alertHistory.unshift(alertMessage); // Add alert to the top
        updateAlertHistory();
    }
}

// Update alert history display
function updateAlertHistory() {
    const alertList = document.getElementById('alert-list');
    alertList.innerHTML = ''; // Clear previous alerts
    alertHistory.forEach(alert => {
        const alertItem = document.createElement('div');
        alertItem.className = 'alert-container';
        alertItem.textContent = alert;
        alertList.appendChild(alertItem);
    });
}

// Update chart with humidity data
function updateChart() {
    const ctx = document.getElementById('humidityChart').getContext('2d');

    if (window.humidityChart instanceof Chart) {
        window.humidityChart.destroy();
    }

    const datasets = humidityData.map((data, index) => ({
        label: `Bitki ${index + 1}`,
        data: data,
        borderColor: colors[index % colors.length],
        backgroundColor: colors[index % colors.length].replace('1)', '0.2)'),
        fill: true,
    }));

    window.humidityChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: timestamps,
            datasets: datasets,
        },
        options: {
            scales: {
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: 'Humidity (%)'
                    }
                },
                x: {
                    title: {
                        display: true,
                        text: 'Time'
                    }
                }
            }
        }
    });
}

// Load layout and initialize
document.addEventListener('DOMContentLoaded', function() {
    const days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    const today = new Date();
    document.getElementById('current-day').textContent = days[today.getDay()];

    setInterval(updateHumidity, 3000); // Update every 3 seconds
    updateHumidity(); // Initial load
});
