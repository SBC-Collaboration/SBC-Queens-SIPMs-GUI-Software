#pragma once

// std includes
#include <algorithm>
#include <ios>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sstream>
#include <deque>

// 3rd party includes
#include <imgui.h>
#include <implot.h>
#include <spdlog/spdlog.h>
#include <readerwriterqueue.h>
#include <concurrentqueue.h>

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
			: ID(id), x(x), y(y) {}
		IndicatorVector(const T& id, const DATA& x)
			: ID(id), x(x) {}

		T ID;
		DATA x;
		DATA y;
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

		// _imgui_stack exists to give this button an unique id
		// for the ImGUI API. If this were not there, it would share
		// an ID with all the empty or same indicators.
		std::string _imgui_stack;

		// Actual string that gets displayed
		std::string _display;

		// Format parameters
		unsigned int _precision;
		NumericFormat _format;

		double val;

protected:

		T ID;

public:

		explicit Indicator(const T& id, const unsigned int& precision = 6,
			const NumericFormat& format = NumericFormat::Default)
			: 	_imgui_stack("##" + std::to_string(static_cast<int>(id))),
				_display(_imgui_stack), _precision(precision), _format(format),
				val(0.0), ID(id)
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

			if(newVal.ID == ID) {

				std::ostringstream out;
				out.precision(_precision);
				if (_format == NumericFormat::Scientific) {
					out << std::scientific;
				} else if (_format == NumericFormat::HexFloat) {
					out << std::hexfloat;
				} else if (_format == NumericFormat::Fixed) {
					out << std::fixed;
				}

				out << newVal.x;
				_display = "";

				if(!out.str().empty()) {
					_display = out.str();
				}

				_display += _imgui_stack;

				val = static_cast<double>(newVal.x);

			}
		}

		virtual void operator()(const std::string& label, double& out) {
			ImGui::Text(label.c_str()); ImGui::SameLine();
			ImGui::Button(_display.c_str());
			out = val;
		}

		virtual void Draw(const std::string& label, double& out) {
			this->operator()(label, out);
		}

		// For indicators, it does nothing.
		virtual void ExecuteAttributes() {

		}

		// For indicators, it does nothing.
		virtual void ClearAttributes() {

		}

		virtual T GetID() const {
			return ID;
		}

	};


	// A plot indicator. It inherits from indicator, but more out of necessity.
	// It keeps the code clean, but it was really not necessary.
	// Originally, a plot could be any type that was not double but
	// ImPlot only shows double numbers or transforms them to doubles
	// internally. Also, x and y have to be the same type.
	template<typename T>
	class Plot : public Indicator<T> {

		std::vector< double > _x;
		std::vector< double > _y;

		bool Cleared = false;
		bool ClearOnNewData = false;

public:
		using type = T;
		using data_type = double;

		explicit Plot(const T& id, bool clearOnNewData = false)
			: Indicator<T>(id), ClearOnNewData(clearOnNewData) {
		}

		// No moving
		Plot(Plot&&) = default;
		Plot& operator=(Plot&&) = default;

		// No copying
		Plot(const Plot&) = delete;

		template<typename OFFDATA>
		void add(const IndicatorVector<T, OFFDATA>& v) {

			if(v.ID == Indicator<T>::ID){
				_x.push_back( static_cast<double>(v.x) );
				_y.push_back( static_cast<double>(v.y) );
			}

		}

		template<typename OFFDATA>
		void add(const OFFDATA& x, const OFFDATA& y) {
			_x.push_back( static_cast<double>(x) );
			_y.push_back( static_cast<double>(y) );
		}

		void clear() {
			_x.clear();
			_y.clear();
		}

		// Returns the start of the X values
		auto begin() const {
			_x.begin();
		}

		// Returns the end of the X values
		auto end() const {
			_x.begin();
		}

		// Executes attributes. For plots, it clears on new data.
		void ExecuteAttributes() {
			if(ClearOnNewData) {
				if(!Cleared) {

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
		void operator()(const std::string& label) {
			ImPlot::PlotLine(label.c_str(), &_x.front(), &_y.front(), _x.size());
		}

		void Draw(const std::string& label) {
			this->operator()(label);
		}

	};

	// This class is meant to be created once in the receiver/consumer thread
	// It will update all of the indicators values once information is sent
	template<typename T, typename DATA>
	class IndicatorReceiver  {

		IndicatorsQueue<T, DATA>& _q;
		std::unordered_map<T, std::unique_ptr<Indicator<T>>> _indicators;
		//std::unordered_map<T, std::unique_ptr<Plot<T>>> _plots;

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

			for(IndicatorVector<T, DATA> item : temp_items) {

				// If it contains one item, then it is a indicator
				if(_indicators.count(item.ID)) {

					auto& indicator = _indicators.at(item.ID);
					indicator->ExecuteAttributes();

					Plot<T>* plt = dynamic_cast<Plot<T>*>(indicator.get());
					if(plt) {
						plt->add(item);
					} else {
						indicator->add(item);
					}
				}
			}

			for(auto& indicator : _indicators) {
				indicator.second->ClearAttributes();
			}

		}

		void operator()(const T& type, const DATA& x, const DATA& y) {
			IndicatorVector<T, DATA> newP(type, x, y);
			_q.enqueue(newP);
		}

		auto& indicator(const T& id,
			const std::string& label,
			double& out,
			const unsigned int& precision = 6,
			const NumericFormat& format = NumericFormat::Default) {

			_indicators.try_emplace(id,
				std::make_unique<Indicator<T>>(id, precision, format));
			auto& ind = _indicators.at(id);

			ind->Draw(label, out);

			return ind;

		}

		auto& indicator(const T& id,
			const std::string& label,
			const unsigned int& precision = 6,
			const NumericFormat& format = NumericFormat::Default) {

			_indicators.try_emplace(id,
				std::make_unique<Indicator<T>>(id, precision, format));
			auto& ind = _indicators.at(id);

			double tmp;
			ind->Draw(label, tmp);

			return ind;

		}

		// auto& make_plot(const T& id) {
		// 	_indicators.try_emplace(id,
		// 		std::make_unique<Plot<T>>(id));

		// 	return _plots.at(id);
		// }

		// Adds plot with ID, and draws it at the placed location if exists
		// It returns a smart pointer to the indicator (not plot)
		auto& plot(const T& id, const std::string& label, bool clearOnNewData = false) {

			_indicators.try_emplace(id,
				std::make_unique<Plot<T>>(id, clearOnNewData));

			auto& ind = _indicators.at(id);

			Plot<T>* plt = dynamic_cast<Plot<T>*>(ind.get());
			if(plt) {
				plt->Draw(label);
			}

			return ind;

		}

	};

	// The IndicatorSender class is meant to be run in the producer threads
	// The producer thread can add a new IndicatorVector to this class
	// and this will send it forward.
	// It uses enums so the producer threads do not need to know if the
	// indicator/plot associated with that enum exists.
	template<typename T, typename DATA>
	class IndicatorSender  {

		IndicatorsQueue<T, DATA>& _q;

public:
		using type = T;
		using data_type = DATA;

		explicit IndicatorSender(IndicatorsQueue<T, DATA>& q) : _q(q) { }

		// No copying
		IndicatorSender(const IndicatorSender&) = delete;

		// Sends value x to indicator of type T
		void operator()(const T& type, const DATA& x) {
			IndicatorVector<T, DATA> newP(type, x);
			_q.enqueue(newP);
		}

		// Sends value pair (x,y) to plot/indicator of type T
		void operator()(const T& type, const DATA& x, const DATA& y) {
			IndicatorVector<T, DATA> newP(type, x, y);
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
		void operator()(const T& type, const std::vector<OFFDATA>& x,
			const std::vector<OFFDATA>& y) {

			std::vector< IndicatorVector<T, DATA> > items(x.size());
			for(unsigned int i = 0; i < x.size(); i++ ) {
				items.emplace(i, type, 
					static_cast<DATA>(x[i]), 
					static_cast<DATA>(y[i]));
			}

			_q.enqueue_bulk(items.begin(), items.size());
		}

		// Sends a list of pairs (x,y) to plot/indicator of type T
		// Note if an indicator is selected, only the latest value
		// will be shown and the list y will be ignored
		template<typename OFFDATA>
		void operator()(const T& type,
			OFFDATA* x_data,
			OFFDATA* y_data,
			const size_t& size) {

			std::vector< IndicatorVector<T, DATA> > items(size);
			for(unsigned int i = 0; i < size; i++ ) {
				items[i] = IndicatorVector<T, DATA>(type, 
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


} // namespace SBCQueens