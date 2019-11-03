#ifndef MEASURER_RANGES_H
#define MEASURER_RANGES_H

#include "util/trace.h"

#include <array>
#include <algorithm>


namespace meter {

class Range {
public:
	// From Espressif ADC documentation: 0dB attenuation (ADC_ATTEN_DB_0) between 100 and 950mV
	static const uint16_t HIGHEST_INPUT_VALUE = 950;
	static const uint16_t LOWEST_INPUT_VALUE = 100;

public:
	Range(): m_factor(0.0), m_zero(0), m_max(0.0), m_min(0.0) {}
	
	Range( uint16_t zero, float factor ): m_factor(factor), m_zero(zero) {
		setZero( zero );
	}

	void setZero( uint16_t zero ) {
		m_max = m_factor * (HIGHEST_INPUT_VALUE-zero);
		m_min = m_factor * (LOWEST_INPUT_VALUE-zero);
		TRACE( "Zero changed: %u -> %u [%f .. %f] (%f)", m_zero, zero, m_min, m_max, m_factor );
		m_zero = zero;
	}

	void setFactor( float factor ) {
		m_max = factor * (HIGHEST_INPUT_VALUE-m_zero);
		m_min = factor * (LOWEST_INPUT_VALUE-m_zero);
		TRACE( "Factor changed: %f -> %f [%f .. %f]", m_factor, factor, m_min, m_max );
		m_factor = factor;
	}

	float zero() const { return m_zero; }
	float max() const { return m_max; }
	float min() const { return m_min; }

	bool contains( float value ) const {
		return (value <= m_max) && (value >= m_min);
	}

	float convert( uint16_t value ) const {
		int16_t raw = value - m_zero;
		return float(raw) * m_factor;
	}

private:
	float m_factor;
	uint16_t m_zero;
	float m_max;
	float m_min;
};


template<size_t N>
class Ranges {
private:
	typedef std::array<Range, N> Container;

	static const uint OverflowsThreshold = 2;
	static const uint ResetOverflowsThreshold = 100;
	static const uint ScoreThreshold = 3000;

public:
	Ranges() {}
	Ranges( const Container& ranges ): 
				m_ranges(ranges), m_active(0), m_overflows(0), m_autoRange(true) {}

    size_t active() const {
		return m_active;
	}

    size_t best() const {
		if ( (m_active > 0) && (m_overflows > OverflowsThreshold) ) {
			return m_active - 1;
		}

		for ( size_t i=N-1; i > m_active; --i ) {
			if (m_scores[i] > ScoreThreshold) {
				return i;
			}
		}
		return m_active;
	}

    bool autoRange() const {
        return m_autoRange;
    }

    void setAutoRange( bool value ) {
        m_autoRange = value;
    }

	void setActive( size_t position ) {
		m_active = position;
		m_overflows = 0;
		m_scores.fill(0);
	}

	float process( uint16_t value ) {
		float volts = m_ranges[m_active].convert(value);
        if ( m_autoRange ) {
            updateScores( volts );
        }
        return volts;
    }

	Range& operator[](size_t index) {
		return m_ranges[index];
	}
	
private:
    void updateScores( float volts ) {
		// Check for overflow. active range should be decremented
		if ( m_ranges[m_active].contains( volts ) ) {
			if ( (m_overflows > 0) && (++m_withoutOverflows > ResetOverflowsThreshold) ) {
				m_overflows = 0;
			}
		}
		else {
			++m_overflows;
			m_withoutOverflows = 0;
		}

		// Check if there is more specific range. active range should be incremented
		for ( size_t i=m_active+1; i<N; ++i ) {
			if ( m_ranges[i].contains( volts ) ) {
				++m_scores[i];
			}
			else {
				m_scores[i] = 0;
			}
		}
	}

private:
	/* const */ Container m_ranges;
	size_t m_active;
	uint m_overflows;
	uint m_withoutOverflows;
    bool m_autoRange;
	std::array<uint, N> m_scores;
};

}

#endif