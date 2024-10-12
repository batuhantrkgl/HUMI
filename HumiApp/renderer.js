// renderer.js

document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM fully loaded and parsed');

    // Display the current day
    const currentDayElement = document.getElementById('current-day');
    const currentDate = new Date();
    currentDayElement.textContent = currentDate.toLocaleDateString('en-US', {
        weekday: 'long', year: 'numeric', month: 'long', day: 'numeric'
    });

    // Button actions
    window.takePhoto = function() {
        console.log('Taking photo...');
    };

    window.waterPlant = function() {
        console.log('Watering plant...');
    };

    window.toggleFan = function() {
        console.log('Toggling fan...');
    };
});
