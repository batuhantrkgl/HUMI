// renderer.js

document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM fully loaded and parsed');

    // Display the current day
    const currentDayElement = document.getElementById('current-day');
    const currentDate = new Date();
    currentDayElement.textContent = currentDate.toLocaleDateString('en-US', {
        weekday: 'long', year: 'numeric', month: 'long', day: 'numeric'
    });

    // Simulate update for plants in dev mode
    const devMode = false; // Set to false for production

    function renderPlants(data) {
        const plantList = document.querySelector('.team-list');
        plantList.innerHTML = ''; // Clear previous plants

        data.forEach((plant, index) => {
            const plantContainer = document.createElement('div');
            plantContainer.classList.add('team-member-container');

            const plantInfo = `
                <div class="team-member-info">
                    <p class="teammate-name">Bitki ${index + 1}</p>
                    <p>Nem Durumu: <span id="humidity-${index + 1}">${plant.humidity}%</span></p>
                    <p>Önerilen Nem: <span>${plant.recommendedHumidity}%</span></p>
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

    // Mock data for development
    if (devMode) {
        const mockData = [
            { humidity: 85, recommendedHumidity: 60 },
            { humidity: 80, recommendedHumidity: 55 },
            { humidity: 85, recommendedHumidity: 70 },
        ];
        renderPlants(mockData);
    }
});

// Actions
window.takePhoto = function() {
    console.log('Taking photo...');
};

window.waterPlant = function(plantNumber) {
    console.log(`Watering plant ${plantNumber}...`);
};

window.toggleFan = function(plantNumber) {
    console.log(`Toggling fan for plant ${plantNumber}...`);
};
