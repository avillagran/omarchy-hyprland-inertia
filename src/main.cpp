// Touchpad inertia for Omarchy. After the fingers lift, keep sending the
// focused surface finger-scroll frames that slow down, for at most 0.8 s.
// It only listens to Hyprland's input events: no function hooks, and nothing
// is sent when no surface has pointer focus.
#include "Momentum.hpp"
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/version.h>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/window/Window.hpp>
#include <hyprland/src/desktop/view/WLSurface.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/managers/SeatManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <array>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
std::array<Momentum, 2> axes;
std::array<Clock::time_point, 2> sampled;
Clock::time_point tickTime;
wl_event_source* timer = nullptr;
WP<CWLSurfaceResource> target;
std::vector<Hyprutils::Signal::CHyprSignalListener> listeners;

double elapsed(Clock::time_point now, Clock::time_point then) {
    return std::chrono::duration<double, std::milli>(now - then).count();
}
void stop() {
    for (auto& axis : axes) axis.reset();
    target.reset();
    if (timer) wl_event_source_timer_update(timer, 0);
}
bool allowed() {
    // Keep native inertia in browsers and terminals; never emit without a target.
    const auto surface = g_pSeatManager->m_state.pointerFocus.lock();
    if (!surface) return false;
    // The window under the pointer, even when it opened under a still cursor.
    // Popups and subsurfaces fall back to the last window the mouse focused.
    const auto hl = Desktop::View::CWLSurface::fromResource(surface);
    auto window = hl ? Desktop::View::CWindow::fromView(hl->view()) : nullptr;
    if (!window) window = g_pInputManager->m_lastMouseFocus.lock();
    if (!window) return false;
    std::string cls = window->metadata().appID();
    std::transform(cls.begin(), cls.end(), cls.begin(), [](unsigned char c) { return std::tolower(c); });
    for (const auto* excluded : {"firefox", "chrom", "brave", "vivaldi", "librewolf", "zen", "kitty", "alacritty", "foot", "ghostty", "terminal"})
        if (cls.find(excluded) != std::string::npos) return false;
    return true;
}
int tick(void*) {
    if (!allowed() || target.expired() || target.lock() != g_pSeatManager->m_state.pointerFocus.lock()) { stop(); return 0; }
    const auto now = Clock::now();
    const auto dt = elapsed(now, tickTime);
    tickTime = now;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    bool active = false;
    static auto factor = CConfigValue<Config::FLOAT>("input:touchpad:scroll_factor");
    for (unsigned i = 0; i < 2; ++i) {
        const auto delta = axes[i].tick(dt);
        if (delta != 0)
            g_pSeatManager->sendPointerAxis(ms, static_cast<wl_pointer_axis>(i), delta * *factor, 0, 0,
                WL_POINTER_AXIS_SOURCE_FINGER, WL_POINTER_AXIS_RELATIVE_DIRECTION_IDENTICAL);
        active |= axes[i].active;
    }
    g_pSeatManager->sendPointerFrame();
    if (active) wl_event_source_timer_update(timer, 16);
    return 0;
}
void axis(const IPointer::SAxisEvent& e, Event::SCallbackInfo& info) {
    if (info.cancelled || e.source != WL_POINTER_AXIS_SOURCE_FINGER || !allowed()) { stop(); return; }
    const unsigned i = static_cast<unsigned>(e.axis);
    if (i > 1 || !std::isfinite(e.delta)) { stop(); return; }
    auto current = g_pSeatManager->m_state.pointerFocus;
    if (target.lock() != current.lock()) stop();
    target = current;
    const auto now = Clock::now();
    const auto dt = elapsed(now, sampled[i]);
    if (e.delta == 0) {
        // Only an explicit finger axis-stop starts momentum. Silence never does.
        axes[i].release(dt);
        if (axes[i].active) {
            tickTime = now;
            wl_event_source_timer_update(timer, 16);
        }
    } else {
        if (axes[0].active || axes[1].active) stop();
        target = current;
        if (dt > 80) axes[i].reset();
        axes[i].sample(e.delta, dt);
        sampled[i] = now;
    }
}
}
APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }
APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE) {
    // Check before registering callbacks. The loader also checks the API ABI.
    if (std::string(__hyprland_api_get_hash()) != __hyprland_api_get_client_hash())
        throw std::runtime_error("omarchy-inertia: incompatible Hyprland ABI");
    timer = wl_event_loop_add_timer(g_pCompositor->m_wlEventLoop, tick, nullptr);
    if (!timer) throw std::runtime_error("omarchy-inertia: timer unavailable");
    listeners.emplace_back(Event::bus()->m_events.input.mouse.axis.listen(axis));
    listeners.emplace_back(Event::bus()->m_events.input.mouse.move.listen([](const Vector2D&, Event::SCallbackInfo&) { stop(); }));
    listeners.emplace_back(Event::bus()->m_events.input.mouse.button.listen([](const IPointer::SButtonEvent&, Event::SCallbackInfo&) { stop(); }));
    listeners.emplace_back(Event::bus()->m_events.input.keyboard.key.listen([](const IKeyboard::SKeyEvent&, Event::SCallbackInfo&) { stop(); }));
    listeners.emplace_back(Event::bus()->m_events.window.active.listen([] { stop(); }));
    // Contact/hold cancels coasting even without a click or new scroll.
    for (const auto& pointer : g_pInputManager->m_pointers) {
        if (!pointer || !pointer->m_isTouchpad) continue;
        listeners.emplace_back(pointer->m_pointerEvents.holdBegin.listen([](const IPointer::SHoldBeginEvent&) { stop(); }));
        listeners.emplace_back(pointer->m_pointerEvents.motion.listen([](const IPointer::SMotionEvent&) { stop(); }));
    }
    return {"omarchy-inertia", "Touchpad inertia for Omarchy", "avillagran", "0.1.0"};
}
APICALL EXPORT void PLUGIN_EXIT() {
    listeners.clear();
    stop();
    if (timer) wl_event_source_remove(timer);
    timer = nullptr;
}
