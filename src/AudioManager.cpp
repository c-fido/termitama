#include "AudioManager.h"
#include <chrono>
#include <thread>

const std::vector<std::string> AudioManager::PLAYLIST = {
    "Pixel Dreams (Lo-fi Hip Hop)",
    "Rainbow Garden (Chiptune)",
    "Midnight Munchies (Synthwave)",
    "Happy Stomps (Upbeat Pop)",
    "Cozy Nap (Ambient)",
    "Adventure Time! (8-bit)",
};

AudioManager::AudioManager() = default;

AudioManager::~AudioManager() {
    stop(); 
}

void AudioManager::start() {
    if (running_.load()) return; 

    running_.store(true);
    currentTrackIdx_.store(0);
    tickCounter_.store(0);

    audioThread_ = std::thread(&AudioManager::audioLoop, this);
}



void AudioManager::stop() {
    running_.store(false);
    if (audioThread_.joinable())
        audioThread_.join();
}


void AudioManager::skipTrack() {
    int next = (currentTrackIdx_.load() + 1) % static_cast<int>(PLAYLIST.size());
    currentTrackIdx_.store(next);
    tickCounter_.store(0);
}


std::string AudioManager::getCurrentTrack() const {
    int idx = currentTrackIdx_.load();
    return PLAYLIST[static_cast<size_t>(idx)];
}


void AudioManager::audioLoop() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!running_.load()) break;

        int tick = tickCounter_.fetch_add(1) + 1;
        if (tick >= TRACK_DURATION_SECS) {
            tickCounter_.store(0);
            int next = (currentTrackIdx_.load() + 1)
                     % static_cast<int>(PLAYLIST.size());
            currentTrackIdx_.store(next);
        }
    }
}
