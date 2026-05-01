#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <vector>


class AudioManager {
public:
    AudioManager();
    ~AudioManager();


    AudioManager(const AudioManager&)            = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void start();
    void stop();
    void skipTrack();  

    std::string getCurrentTrack() const;
    bool        isPlaying()       const { return running_.load(); }

private:

    static const std::vector<std::string> PLAYLIST;
    static constexpr int TRACK_DURATION_SECS = 30; 

    std::thread           audioThread_;
    std::atomic<bool>     running_{false};
    std::atomic<int>      currentTrackIdx_{0};
    std::atomic<int>      tickCounter_{0};    

    void audioLoop();
};
