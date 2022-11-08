#ifndef IMPLOTHELPERS_H
#define IMPLOTHELPERS_H
#pragma once

// C 3rd party includes
#include <imgui.h>
#include <implot.h>
#include <spdlog/spdlog.h>
#include <readerwriterqueue.h>
#include <concurrentqueue.h>

// C++ STD includes
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ios>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sstream>
#include <deque>

// my includes

namespace SBCQueens {

template<typename T, typename DATA = double>
class IndicatorReceiver;
template<typename T, typename DATA = double>
class IndicatorSender;

// Well, I will leave this as this until I can come up with
// a better solution.
// ImPlots are 2D by default. X, Y are the types of those two
// axis which by default are float but they can be changed.
// unsigned int is a number that instructs the consumer
// to which graph is should go to.
// T is reserved for the user, it could be an enum type (my preference)
// or anything they want to distinguish them.
template<typename T, typename DATA = double>
struct IndicatorVector {
    using type = T;
    using data_type = DATA;

    IndicatorVector() {}
    IndicatorVector(const T& id, const DATA& x, const DATA& y)
        : ID(id), X(x), Y(y) {}
    IndicatorVector(const T& id, const DATA& x)
        : ID(id), X(x) {}

    T ID;
    DATA X;
    DATA Y;
};

template<typename T, typename DATA = double>
using IndicatorsQueue
    = moodycamel::ConcurrentQueue< IndicatorVector<T, DATA> >;

struct PlotFlags {
    bool ClearOnNewData;
};

enum class NumericFormat {
    Default, Scientific, HexFloat, Fixed
};

// An indicator is a ImGUI button that has been modified to act
// like an indicator.
// As for now, this indicator has some parameters such as
// precision of the number to display, and format of the number
template<typename T>
class Indicator {
 protected:
    T ID;
    // _imgui_stack exists to give this button an unique id
    // for the ImGUI API. If this were not there, it would share
    // an ID with all the empty or same indicators.
    std::string _imgui_stack;

 private:
    // Actual string that gets displayed
    std::string _display;

    // Format parameters
    unsigned int _precision;
    NumericFormat _format;

    double _val;

 public:
    explicit Indicator(const T& id, const unsigned int& precision = 6,
        const NumericFormat& format = NumericFormat::Default)
        :   ID(id),
            _imgui_stack("##" + std::to_string(static_cast<int>(id))),
            _display(_imgui_stack), _precision(precision), _format(format),
            _val(0.0)
        { }

    // Moving allowed
    Indicator(Indicator&&) = default;
    Indicator& operator=(Indicator&&) = default;

    // No copying
    Indicator(const Indicator&) = delete;

    virtual ~Indicator() {}

    template<typename OFFDATA>
    // Adds element newVal.x to show in this indicator. newVal.y is ignored
    void add(const IndicatorVector<T, OFFDATA>& newVal) {
        if (newVal.ID == ID) {
            std::ostringstream out;
            out.precision(_precision);
            if (_format == NumericFormat::Scientific) {
                out << std::scientific;
            } else if (_format == NumericFormat::HexFloat) {
                out << std::hexfloat;
            } else if (_format == NumericFormat::Fixed) {
                out << std::fixed;
            }

            out << newVal.X;
            _display = "";

            if (!out.str().empty()) {
                _display = out.str();
            }

            _display += _imgui_stack;

            _val = static_cast<double>(newVal.X);
        }
    }

    template<typename OFFDATA>
    void operator()(const std::string& label, OFFDATA& out,
        const float& offset = 0) {
        ImGui::Text("%s", label.c_str()); ImGui::SameLine(offset);
        ImGui::Button(_display.c_str());
        out = static_cast<OFFDATA>(_val);
    }

    template<typename OFFDATA>
    void Draw(const std::string& label, OFFDATA& out, const float& offset = 0) {
        this->operator()(label, out, offset);
    }

    // For indicators, it does nothing.
    virtual void ExecuteAttributes() { }

    // For indicators, it does nothing.
    virtual void ClearAttributes() { }

    virtual T GetID() const {
        return ID;
    }
};

template<typename T>
class BooleanIndicator : public Indicator<T> {
    std::function<bool(const double&)> _f;
    ImVec4 _off_color;
    ImVec4 _on_color;
    ImVec4 _current_color;
    bool _is_on;

 public:
    explicit BooleanIndicator(const T& id,
        std::function<bool(const double&)>&& f)
        : Indicator<T>(id), _f(f),
        _off_color(static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, 0.6f))),
        _on_color(static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.6f, 0.6f))),
        _current_color(_off_color), _is_on(false) {
    }

    // Moving allowed
    BooleanIndicator(BooleanIndicator&&) = default;
    BooleanIndicator& operator=(BooleanIndicator&&) = default;

    // No copying
    BooleanIndicator(const BooleanIndicator&) = delete;

    template<typename OFFDATA>
    // Uses newVal.x to indicate the equality
    void add(const IndicatorVector<T, OFFDATA>& newVal) {
        _is_on = _f(static_cast<double>(newVal.X));
        _current_color = _is_on ? _on_color : _off_color;
    }

    void operator()(const std::string& label, bool& out,
        const float& offset = 0) {
        out = _is_on;
        ImGui::Text("%s", label.c_str()); ImGui::SameLine(offset);

        ImGui::PushStyleColor(ImGuiCol_Button, _current_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, _current_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, _current_color);

        ImGui::Button(Indicator<T>::_imgui_stack.c_str(), ImVec2(15, 15));

        ImGui::PopStyleColor(3);
    }

    void Draw(const std::string& label, bool& out, const float& offset = 0) {
        this->operator()(label, out, offset);
    }
};

// Internal structure to be used for Plot class. Servers as an interface
// between this API and ImPlot
struct PlotDataBuffer {
    const std::size_t N;
    std::size_t Start = 0;
    std::size_t Size = 0;
    std::size_t CurrentIndex = 0;

    std::vector< std::pair <double, double> > Data;

    explicit PlotDataBuffer(const std::size_t& max)
        : N(max), Data(max) {}

    std::pair<double, double>& operator[](const std::size_t& i) {
        std::size_t index = (i + Start) % Size;
        return Data[index];
    }

    void operator()(const double& x, const double& y) {
        Data[CurrentIndex] = std::make_pair(
                static_cast<double>(x),
                static_cast<double>(y));

        if (Size < N) {
            Size++;
            CurrentIndex = Size == N ? 0 : CurrentIndex + 1;
        } else {
            Start = (Start + 1) % Size;
            CurrentIndex = Start;
        }
    }
};

enum class PlotStyle {
    Line,
    Scatter
};
// A plot indicator. It inherits from indicator, but more out of necessity.
// It keeps the code clean, but it was really not necessary.
// Originally, a plot could be any type that was not double but
// ImPlot only shows double numbers or transforms them to doubles
// internally. Also, x and y have to be the same type.
template<typename T>
class Plot : public Indicator<T> {
    bool Cleared = false;
    bool ClearOnNewData = false;


    const std::size_t MaxSamples;
    PlotDataBuffer plotData;

 public:
    using type = T;
    using data_type = double;

    explicit Plot(const T& id, const bool& clearOnNewData = false,
        const std::size_t& maxSamples = 100e3) : Indicator<T>(id),
        ClearOnNewData(clearOnNewData), MaxSamples(maxSamples),
        plotData(maxSamples) {
    }

    // No moving
    Plot(Plot&&) = default;
    Plot& operator=(Plot&&) = default;

    // No copying
    Plot(const Plot&) = delete;

    template<typename OFFDATA>
    void add(const IndicatorVector<T, OFFDATA>& v) {
        if (v.ID == Indicator<T>::ID) {
            plotData(v.X, v.Y);
        }
    }

    template<typename OFFDATA>
    void add(const OFFDATA& x, const OFFDATA& y) {
        plotData(x, y);
    }

    void clear() {
        plotData.Size = 0;
        plotData.CurrentIndex = 0;
    }

    // Executes attributes. For plots, it clears on new data.
    void ExecuteAttributes() {
        if (ClearOnNewData) {
            if (!Cleared) {
                clear();
                Cleared = true;
            }
        }
    }

    // Clears attributes. For plots, it resets if it should clear.
    // on new data.
    void ClearAttributes() {
        Cleared = false;
    }

    // Wraps ImPlot::PlotLine
    void operator()(const std::string& label,
        const PlotStyle& style = PlotStyle::Line) {
        // static ImPlotGetter getter = static_cast<ImPlotGetter>(_transform);
        static auto transform = [](int idx, void* data) {
            auto myData = static_cast<PlotDataBuffer*>(data);
            auto pair = (*myData)[idx];
            return ImPlotPoint(pair.first, pair.second);
        };

        switch (style) {
            case PlotStyle::Scatter:
                ImPlot::PlotScatterG(label.c_str(), transform, &plotData, plotData.Size);
            break;
            case PlotStyle::Line:
            default:
                ImPlot::PlotLineG(label.c_str(), transform, &plotData, plotData.Size);
            break;
        }

    }

    void Draw(const std::string& label,
        const PlotStyle& style = PlotStyle::Line) {
        this->operator()(label, style);
    }

 private:
};

// This class is meant to be created once in the receiver/consumer thread
// It will update all of the indicators values once information is sent
template<typename T, typename DATA>
class IndicatorReceiver  {
    IndicatorsQueue<T, DATA>& _q;
    std::unordered_map<T, std::unique_ptr<Indicator<T>>> _indicators;

public:
    using type = T;
    using data_type = DATA;

    explicit IndicatorReceiver(IndicatorsQueue<T, DATA>& q) : _q(q) { }


    // No copying
    IndicatorReceiver(const IndicatorReceiver&) = delete;

    // This is meant to be run in the single consumer.
    // Updates all the plots arrays from the queue.
    void operator()() {
        auto approx_length = _q.size_approx();
        std::vector< IndicatorVector<T, DATA> > temp_items(approx_length);
        _q.try_dequeue_bulk(temp_items.data(), approx_length);

        for (IndicatorVector<T, DATA> item : temp_items) {
            // If it contains one item, then it is a indicator
            if (_indicators.count(item.ID)) {
                auto& indicator = _indicators.at(item.ID);
                indicator->ExecuteAttributes();

                Plot<T>* plt = dynamic_cast<Plot<T>*>(indicator.get());
                BooleanIndicator<T>* bind
                    = dynamic_cast<BooleanIndicator<T>*>(indicator.get());
                if (plt) {
                    plt->add(item);
                } else if (bind) {
                    bind->add(item);
                } else {
                    indicator->add(item);
                }
            }
        }

        for (auto& indicator : _indicators) {
            indicator.second->ClearAttributes();
        }
    }

    void operator()(const T& key, const DATA& x, const DATA& y) {
        IndicatorVector<T, DATA> newP(key, x, y);
        _q.enqueue(newP);
    }

    template<typename OFFDATA>
    auto& indicator(const T& id, const std::string& label, OFFDATA& out,
        const float& offset = 0,
        const unsigned int& precision = 6,
        const NumericFormat& format = NumericFormat::Default) {
        _indicators.try_emplace(id,
            std::make_unique<Indicator<T>>(id, precision, format));

        auto& ind = _indicators.at(id);

        ind->Draw(label, out, offset);

        return ind;
    }

    auto& indicator(const T& id,
        const std::string& label,
        const float& offset = 0,
        const unsigned int& precision = 6,
        const NumericFormat& format = NumericFormat::Default) {

        _indicators.try_emplace(id,
            std::make_unique<Indicator<T>>(id, precision, format));
        auto& ind = _indicators.at(id);

        double tmp;
        ind->Draw(label, tmp, offset);

        return ind;
    }

    auto& booleanIndicator(const T& id, const std::string& label,
        std::function<bool(const double&)>&& f, const float& offset = 0) {
        _indicators.try_emplace(id,
            std::make_unique<BooleanIndicator<T>>(id,
                std::forward<std::function<bool(const double&)>>(f)));

        auto& ind = _indicators.at(id);

        auto b = dynamic_cast<BooleanIndicator<T>*>(ind.get());
        if (b) {
            b->Draw(label, offset);
        }

        return ind;
    }

    auto& booleanIndicator(const T& id, const std::string& label, bool& out,
        std::function<bool(const double&)>&& f, const float& offset = 0) {
        _indicators.try_emplace(id,
            std::make_unique<BooleanIndicator<T>>(id,
                std::forward<std::function<bool(const double&)>>(f)));

        auto& ind = _indicators.at(id);

        auto b = dynamic_cast<BooleanIndicator<T>*>(ind.get());
        if (b) {
            b->Draw(label, out, offset);
        }

        return ind;
    }

    // auto& make_plot(const T& id) {
    //  _indicators.try_emplace(id,
    //      std::make_unique<Plot<T>>(id));

    //  return _plots.at(id);
    // }

    // Adds plot with ID, and draws it at the placed location if exists
    // It returns a smart pointer to the indicator (not plot)
    auto& plot(const T& id, const std::string& label,
        const PlotStyle style = PlotStyle::Line, bool clearOnNewData = false) {
        _indicators.try_emplace(id,
            std::make_unique<Plot<T>>(id, clearOnNewData));

        auto& ind = _indicators.at(id);

        Plot<T>* plt = dynamic_cast<Plot<T>*>(ind.get());
        if (plt) {
            plt->Draw(label, style);
        }

        return ind;
    }

    void ClearPlot(const T& id) {
        auto search = _indicators.find(id);
        if (search != _indicators.end()) {
            Plot<T>* plt = dynamic_cast<Plot<T>*>(
                _indicators.at(id).get());
            plt->clear();
        }
    }
};

// The IndicatorSender class is meant to be run in the producer threads
// The producer thread can add a new IndicatorVector to this class
// and this will send it forward.
// It uses enums so the producer threads do not need to know if the
// indicator/plot associated with that enum exists.
template<typename T, typename DATA>
class IndicatorSender {
    IndicatorsQueue<T, DATA>& _q;

 public:
    using type = T;
    using data_type = DATA;

    explicit IndicatorSender(IndicatorsQueue<T, DATA>& q) : _q(q) { }

    // No copying
    IndicatorSender(const IndicatorSender&) = delete;

    // Sends value x to indicator of type T
    void operator()(const T& key, const DATA& x) {
        IndicatorVector<T, DATA> newP(key, x);
        _q.enqueue(newP);
    }

    // Sends value pair (x,y) to plot/indicator of type T
    void operator()(const T& key, const DATA& x, const DATA& y) {
        IndicatorVector<T, DATA> newP(key, x, y);
        _q.enqueue(newP);
    }

    // Sends value pair (x,y) to plot/indicator of type T
    // Note if an indicator is selected, only the latest value
    // will be shown
    void operator()(const IndicatorVector<T, DATA>& pv) {
        _q.enqueue(pv);
    }

    // Sends a list of pairs (x,y) to plot/indicator of type T
    // Note if an indicator is selected, only the latest value
    // will be shown and the list y will be ignored
    template<typename OFFDATA>
    void operator()(const T& key, const std::vector<OFFDATA>& x,
        const std::vector<OFFDATA>& y) {
        std::vector< IndicatorVector<T, DATA> > items(x.size());
        for (unsigned int i = 0; i < x.size(); i++) {
            items[i] = IndicatorVector<T, DATA>(key,
                static_cast<DATA>(x[i]),
                static_cast<DATA>(y[i]));
        }

        _q.enqueue_bulk(items.begin(), items.size());
    }

    // Sends a list of pairs (x,y) to plot/indicator of type T
    // Note if an indicator is selected, only the latest value
    // will be shown and the list y will be ignored
    template<typename OFFDATA>
    void operator()(const T& key, OFFDATA* x_data, OFFDATA* y_data,
        const size_t& size) {
        std::vector< IndicatorVector<T, DATA> > items(size);
        for (unsigned int i = 0; i < size; i++) {
            items[i] = IndicatorVector<T, DATA>(key,
                static_cast<DATA>(x_data[i]),
                static_cast<DATA>(y_data[i]));
        }

        _q.enqueue_bulk(items.begin(), items.size());
    }
};


template<typename T>
// Creates indicator with Key and adds it to Receiver q
inline Indicator<T> make_indicator(T&& key, unsigned int&& precision = 6,
        NumericFormat&& format = NumericFormat::Default) {
    return Indicator<T>(std::forward<T>(key),
        std::forward<unsigned int>(precision),
        std::forward<NumericFormat>(format));
}

template<typename T>
inline Plot<T> make_plot(T&& key) {
    return Plot<T>(std::forward<T>(key));
}


}  // namespace SBCQueens
#endif
