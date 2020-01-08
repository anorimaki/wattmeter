#ifndef MEASURER_RANGES_H
#define MEASURER_RANGES_H

#include "util/trace.h"

#include <array>
#include <algorithm>


namespace meter {


class Range {
public:
	// From Espressif ADC documentation: 0dB attenuation (ADC_ATTEN_DB_0) between 100 and 950mV
	static const uint16_t HIGHEST_INPUT_VALUE = 9500;
	static const uint16_t LOWEST_INPUT_VALUE = 1000;

public:
    uint16_t zero() const { return m_zero; }
    float scaleFactor() const { return m_scaleFactor; }

	void setZero( uint16_t zero ) {
		TRACE( "Zero changed: %u -> %u", m_zero, zero );
		m_zero = zero;
	}

	void setFactors( float factor, float underflowFactor ) {
		TRACE( "Factor changed: %f -> %f", m_scaleFactor, factor );
		m_scaleFactor = factor;
        setUnderflowFactor( underflowFactor );
	}

    bool isUnderflow( uint16_t rawValue ) const {
        int16_t value = rawValue - m_zero;
        return (m_underflowMax > value) && (m_underflowMin < value);
    }

	int16_t applyOffset( uint16_t value ) const {
		return value - m_zero;
	}

    static bool isOverflow( uint16_t value ) {
        return  (LOWEST_INPUT_VALUE > value) || (HIGHEST_INPUT_VALUE < value);
    }

private:
    void setUnderflowFactor( float underflowScaleFactor ) {
        int16_t max = HIGHEST_INPUT_VALUE - m_zero;
        int16_t min = LOWEST_INPUT_VALUE - m_zero;
        float p = underflowScaleFactor / m_scaleFactor;
        m_underflowMax = p * max;
        m_underflowMin = p * min;
        TRACE( "Underflow limits: [%d .. %d]", m_underflowMin, m_underflowMax );
    }

private:
	float m_scaleFactor;
	uint16_t m_zero;
	int16_t m_underflowMax;
	int16_t m_underflowMin;
};


template<size_t N>
class Ranges {
private:
	typedef std::array<Range, N> Container;

	static const uint OverflowsThreshold = 2;
	static const uint ResetOverflowsThreshold = 100;
	static const uint UnderflowsThreshold = 3000;

public:
    template <typename C>
    void setZeros( const C& zeros ) {
        for( uint i = 0; i < N; ++i ) {
            m_ranges[i].setZero( zeros[i] );
        }
    }

    template <typename C>
    void setScaleFactors( const C& scaleFactors ) {
        for( uint i = 0; i < N-1; ++i ) {
            m_ranges[i].setFactors( scaleFactors[i], scaleFactors[i+1] );
        }
        m_ranges[N-1].setFactors( scaleFactors[N-1], 0.0 );
    }

    size_t active() const {
		return m_active;
	}

    size_t best() const {
		if ( (m_active > 0) && (m_overflows > OverflowsThreshold) ) {
			return m_active - 1;
		}
        if ( m_underflows > UnderflowsThreshold ) {
            return m_active + 1;
        }
		return m_active;
	}

    bool autoRange() const {
        return m_autoRange;
    }

    float scaleFactor() const {
        return m_ranges[m_active].scaleFactor();
    }

    void setAutoRange( bool value ) {
        m_autoRange = value;
    }

	void setActive( size_t position ) {
		m_active = position;
		m_overflows = 0;
		m_underflows = 0;
	}

	int16_t process( uint16_t value ) {
		int16_t volts = m_ranges[m_active].applyOffset(value);
        if ( m_autoRange ) {
            updateScores( value );
        }
        return volts;
    }

	const Range& operator[](size_t index) const {
		return m_ranges[index];
	} 

private:
    void updateScores( uint16_t volts ) {
        if ( Range::isOverflow( volts ) ) {
            ++m_overflows;
			m_withoutOverflows = 0;
		}
		else {
			if ( (m_overflows > 0) && (++m_withoutOverflows > ResetOverflowsThreshold) ) {
				m_overflows = 0;
			}
		}

        if ( m_ranges[m_active].isUnderflow( volts ) ) {
            ++m_underflows;
        }
        else {
            m_underflows = 0;
        }
	}

private:
	Container m_ranges;
	size_t m_active;
	uint m_overflows;
    uint m_underflows;
	uint m_withoutOverflows;
    bool m_autoRange;
	std::array<uint, N> m_scores;
};

}

#endif