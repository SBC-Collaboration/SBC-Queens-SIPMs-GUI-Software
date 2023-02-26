#include "sbcqueens-gui/gui_windows/RunTab.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <unordered_map>

// C++ 3rd party includes
#include <imgui.h>
#include <implot.h>

// my includes
#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"
#include "sbcqueens-gui/imgui_helpers.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <unordered_map>

#include "sbcqueens-gui/gui_windows/ControlList.hpp"
#include "sbcqueens-gui/gui_windows/IndicatorList.hpp"

namespace SBCQueens {

void RunTab::init_tab(const toml::table& cfg) {
    auto t_conf = cfg["Teensy"];
    auto CAEN_conf = cfg["CAEN"];
    auto other_conf = cfg["Other"];
    auto file_conf = cfg["File"];
	i_run_dir  = file_conf["RunDir"].value_or("./RUNS");

	_teensy_doe.RunDir = i_run_dir;
	_sipm_doe.RunDir = i_run_dir;
	_slowdaq_doe.RunDir = i_run_dir;

    // Teens stuff
    _teensy_doe.Port = t_conf["Port"].value_or("COM3");

    // CAEN Run stuff
    std::unordered_map<std::string, CAENConnectionType> connection_type_map =
      	{{"USB", CAENConnectionType::USB, },
        {"A4818", CAENConnectionType::A4818}};
    _sipm_doe.Model
        = CAENDigitizerModelsMap.at(CAEN_conf["Model"].value_or("DT5730B"));
    _sipm_doe.PortNum
        = CAEN_conf["Port"].value_or(0u);
    _sipm_doe.ConnectionType
      = connection_type_map[CAEN_conf["ConnectionType"].value_or("USB")];
    _sipm_doe.VMEAddress
        = CAEN_conf["VMEAddress"].value_or(0u);

    // Other/slow daq stuff
    _slowdaq_doe.PFEIFFERPort
  		= other_conf["PFEIFFERSingleGauge"]["Port"].value_or("COM5");

    std::size_t pressure_buffer_size
        = other_conf["PFEIFFERSingleGauge"]["PlotSize"].value_or(86400);
    _slowdaq_doe.PressureData = PlotDataBuffer<1>(pressure_buffer_size);
}

void RunTab::draw() {
	ImGui::InputText("Runs Directory", &i_run_dir);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Directory of where all the run "
            "files are going to be saved at.\n"
            "Fixed when the connect buttons are pressed.");
    }

    ImGui::Separator();
    ImGui::PushItemWidth(100);

    // Com port text input
    // We do not make this a GUI queue item because
    // we only need to let the client know when we click connnect
    ImGui::InputText("Teensy COM port", &_teensy_doe.Port);

    ImGui::SameLine(300);

    static bool tmp;
    constexpr auto connect_teensy = get_control<ControlTypes::Button,
                                    "Connect##Teensy">(SiPMGUIControls);
    draw_control(connect_teensy, _teensy_doe,
        tmp, [&](){ return tmp; },
        // Callback when IsItemEdited !
        [doe = _teensy_doe, run_dir = i_run_dir]
        (TeensyControllerData& teensy_twin) {
            teensy_twin.RunDir = run_dir;
            teensy_twin.Port = doe.Port;
            teensy_twin.CurrentState
                = TeensyControllerStates::AttemptConnection;
    });

    /// Disconnect button
    ImGui::SameLine();

    constexpr Control disconnect_teensy = get_control<ControlTypes::Button,
                                    "Disconnect##Teensy">(SiPMGUIControls);
    draw_control(disconnect_teensy, _teensy_doe,
        tmp, [&](){ return tmp; },
        // Callback when IsItemEdited !
        [](TeensyControllerData& teensy_twin) {
            if (teensy_twin.CurrentState == TeensyControllerStates::Connected) {
                teensy_twin.CurrentState
                    = TeensyControllerStates::Disconnected;
            }
    });

    ImGui::SameLine();

    constexpr auto teensy_connected_led = get_indicator<IndicatorTypes::LED,
                                        "##Teensy Connected?">(SiPMGUIIndicators);
    draw_indicator(teensy_connected_led, _teensy_doe.CurrentState,
        [](const TeensyControllerStates& state) -> bool {
            return state != TeensyControllerStates::Standby and
                state != TeensyControllerStates::AttemptConnection;
    });

    ImGui::Separator();

    constexpr Control sipm_port = get_control<ControlTypes::InputInt,
                                        "CAEN Port">(SiPMGUIControls);
    draw_control(sipm_port, _sipm_doe,
        _sipm_doe.PortNum, ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& caen_twin) {
          caen_twin.PortNum = _sipm_doe.PortNum;
    });

    ImGui::SameLine();

    static std::unordered_map<CAENConnectionType, std::string> model_map =
        {{CAENConnectionType::USB, "USB"},
        {CAENConnectionType::A4818, "A4818 USB 3.0 to CONET Adapter"}};
    constexpr auto connection_type =
        get_control<ControlTypes::ComboBox, "Connection Type">(SiPMGUIControls);
    draw_control(connection_type, _sipm_doe,
        _sipm_doe.ConnectionType, ImGui::IsItemDeactivatedAfterEdit,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& caen_twin) {
          caen_twin.ConnectionType = _sipm_doe.ConnectionType;
        }, model_map
    );

    if (_sipm_doe.ConnectionType == CAENConnectionType::A4818) {
        ImGui::SameLine();
        // ImGui::InputScalar("VME Address", ImGuiDataType_U32,
        //     &cgui_state.VMEAddress);
        constexpr auto caen_model = get_control<ControlTypes::InputUINT32,
            "VME Address">(SiPMGUIControls);
        draw_control(caen_model, _sipm_doe,
            _sipm_doe.VMEAddress, ImGui::IsItemEdited,
            // Callback when IsItemEdited !
            [&](SiPMAcquisitionData& caen_twin) {
              caen_twin.VMEAddress = _sipm_doe.VMEAddress;
        });

    }

    constexpr auto keith_com = get_control<ControlTypes::InputText,
                                    "Keithley COM Port">(SiPMGUIControls);
    draw_control(keith_com, _sipm_doe,
        _sipm_doe.SiPMVoltageSysPort, ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& caen_twin) {
          caen_twin.SiPMVoltageSysPort = _sipm_doe.SiPMVoltageSysPort;
    });

    ImGui::SameLine(300);
    // Colors to pop up or shadow it depending on the conditions

    // This button starts the CAEN communication and sends all
    // the setup configuration
    constexpr auto connect_caen = get_control<ControlTypes::Button,
                                        "Connect##CAEN">(SiPMGUIControls);
    draw_control(connect_caen, _sipm_doe,
        tmp, [&](){ return tmp; },
        // Callback when tmp is true !
        [doe = _sipm_doe, run_dir = i_run_dir]
        (SiPMAcquisitionData& caen_twin) {
            caen_twin = doe;

            caen_twin.RunDir = run_dir;
            caen_twin.CurrentState = SiPMAcquisitionManagerStates::Acquisition;
            caen_twin.AcquisitionState = SiPMAcquisitionStates::Oscilloscope;
    });
    ImGui::SameLine();

    constexpr auto disconnect_caen_btn = get_control<ControlTypes::Button,
                                        "Disconnect##CAEN">(SiPMGUIControls);
    draw_control(disconnect_caen_btn, _sipm_doe,
        tmp, [&](){ return tmp; },
        // Callback when tmp is true !
        [](SiPMAcquisitionData& caen_twin) {
            if(caen_twin.CurrentState == SiPMAcquisitionManagerStates::Acquisition) {
                caen_twin.CurrentState = SiPMAcquisitionManagerStates::Standby;
            }
    });

    ImGui::SameLine();

    constexpr auto caen_connected_led = get_indicator<IndicatorTypes::LED,
                                        "##CAEN Connected?">(SiPMGUIIndicators);
    draw_indicator(caen_connected_led, _sipm_doe.CurrentState,
        [](const SiPMAcquisitionManagerStates& state) -> bool {
            return state != SiPMAcquisitionManagerStates::Standby;
    });

    ImGui::Separator();

    constexpr auto pfeiffer_port = get_control<ControlTypes::InputText,
                                        "PFEIFFER Port">(SiPMGUIControls);
    draw_control(pfeiffer_port, _slowdaq_doe,
        _slowdaq_doe.PFEIFFERPort, ImGui::IsItemEdited,
        // Callback when tmp is true !
        [&](SlowDAQData& doe_twin) {
            doe_twin.PFEIFFERPort = _slowdaq_doe.PFEIFFERPort;
    });

    ImGui::SameLine(300);

    constexpr auto connect_slowdaq = get_control<ControlTypes::Button,
                                    "Connect##SLOWDAQ">(SiPMGUIControls);
    draw_control(connect_slowdaq, _slowdaq_doe,
        tmp, [&](){ return tmp; },
        // Callback when tmp is true !
        [](SlowDAQData& doe_twin) {
            doe_twin.PFEIFFERState = PFEIFFERSSGState::AttemptConnection;
    });

    // ImGui::PopStyleColor(3);

    /// Disconnect button
    ImGui::SameLine();

    constexpr auto disconnect_slowdaq = get_control<ControlTypes::Button,
                                    "Disconnect##SLOWDAQ">(SiPMGUIControls);
    draw_control(disconnect_slowdaq, _slowdaq_doe,
        tmp, [&](){ return tmp; },
        // Callback when tmp is true !
        [](SlowDAQData& doe_twin) {
            if (doe_twin.PFEIFFERState == PFEIFFERSSGState::AttemptConnection ||
                doe_twin.PFEIFFERState == PFEIFFERSSGState::Connected){
                    doe_twin.PFEIFFERState = PFEIFFERSSGState::Disconnected;
            }
    });

    ImGui::SameLine();

    constexpr auto slow_daq_led = get_indicator<IndicatorTypes::LED,
                                        "##Other Connected?">(SiPMGUIIndicators);
    draw_indicator(slow_daq_led, _slowdaq_doe.PFEIFFERState,
        [](const PFEIFFERSSGState& state) -> bool {
            return state != PFEIFFERSSGState::Standby;
    });
    // ImGui::PopStyleColor(3);

    ImGui::PopItemWidth();

    ImGui::Separator();
}

} // namespace SBCQueens