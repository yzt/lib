#pragma once

namespace y {
    namespace Stats {

template <typename SampleType = double, typename AggregateType = double, typename CountType = unsigned long long>
class Bunch {
public:
    bool empty () const {return m_count == 0;}

    CountType count () const {return m_count;}
    SampleType min () const {return m_min;}
    SampleType max () const {return m_max;}
    AggregateType sum () const {return m_sum;}
    AggregateType mean () const {return (m_count != 0) ? (m_sum / m_count) : 0;}

    void addSample (SampleType sample) {
        ++m_count;
        if (sample < m_min) m_min = sample;
        if (m_max < sample) m_max = sample;
        m_sum += sample;
    }

    void addBunch (Bunch const & bunch) {
        m_count += bunch.m_count;
        if (bunch.m_min < m_min) m_min = bunch.m_min;
        if (m_max < bunch.m_max) m_max = bunch.m_max;
        m_sum += bunch.m_sum;
    }

private:
    CountType m_count = 0;
    SampleType m_min =  1e300 * 1e300;  // 1.0 / 0.0;
    SampleType m_max = -1e300 * 1e300;  //-1.0 / 0.0;
    AggregateType m_sum = 0;
    // AggregateType m_sdev = 0.0;
    // ...
};

template <int Length, typename SampleType = double, typename AggregateType = double, typename CountType = unsigned long long>
class Scope {
public:
    using BunchType = Bunch<SampleType, AggregateType, CountType>;

public:
    void addSample (SampleType sample) {m_bunches[0].addSample(sample);}
    void addBunch (BunchType const & bunch) {m_bunches[0].addBunch(bunch);}

    constexpr int size () const {return Length;}
    BunchType & bunch (int index) {return m_bunches[index];}
    BunchType const & bunch (int index) const {return m_bunches[index];}

    int now () const {return m_now;}

    bool shift () { // returns true iff there was "carry", i.e. now() wrapped around.
        m_now = ((m_now + 1 < Length) ? m_now + 1 : 0);
        for (int i = N - 1; i > 0; --i)
            m_bunches[i] = m_bunches[i - 1];
        m_bunches[0] = {};
        return 0 == m_now;
    }

private:
    int m_now = 0;
    BunchType m_bunches [Length];
};

template <int TicksPerSecond, int DaysToTrack = 7, typename SampleType = double, typename AggregateType = double, typename CountType = unsigned long long>
class Timeline {
    using ST = SampleType;
    using AT = AggregateType;
    using CT = CountType;
    static constexpr int TC = TicksPerSecond;
    static constexpr int DC = DaysToTrack;

public:
    auto const & subseconds () const {return m_subseconds;}
    auto const & seconds () const {return m_seconds;}
    auto const & minutes () const {return m_minutes;}
    auto const & hours () const {return m_hours;}
    auto const & days () const {return m_days;}

    void addSample (SampleType sample) {
        m_subseconds.addSample(sample);
        m_seconds.addSample(sample);
        m_minutes.addSample(sample);
        m_hours.addSample(sample);
        m_days.addSample(sample);
    }

    void tick () {
        m_subseconds.shift() &&
        m_seconds.shift() &&
        m_minutes.shift() &&
        m_hours.shift() &&
        m_days.shift();
    }

private:
    Scope<TC, ST, AT, CT> m_subseconds;
    Scope<60, ST, AT, CT> m_seconds;
    Scope<60, ST, AT, CT> m_minutes;
    Scope<24, ST, AT, CT> m_hours;
    Scope<DC, ST, AT, CT> m_days;
};

class Metadata {
    template <int N>    // Make sure you have room for N+1 characters!
    static void StrCopy (char * dst, char const * src) {
        if (src)
            for (int i = 0; i < N && *src; ++i, ++src, ++dst)
                *dst = *src;
        *dst = '\0';
    }

public:
    static constexpr int MaxTypeLen = 63;       // e.g. "incoming bandwidth"
    static constexpr int MaxOwnerLen = 63;      // e.g. "object state data"
    static constexpr int MaxUnitLen = 31;       // e.g. "bytes"
    static constexpr int MaxCategoryLen = 31;   // e.g. "network"

public:
    char const * type () const {return m_type;}
    char const * owner () const {return m_owner;}
    char const * unit () const {return m_unit;}
    char const * category () const {return m_category;}

    void setType (char const * new_type) {StrCopy<MaxTypeLen>(m_type, new_type);}
    void setOwner (char const * new_owner) {StrCopy<MaxOwnerLen>(m_owner, new_owner);}
    void setUnit (char const * new_unit) {StrCopy<MaxUnitLen>(m_unit, new_unit);}
    void setCategory (char const * new_category) {StrCopy<MaxCategoryLen>(m_category, new_category);}

    void setAll (char const * new_type, char const * new_owner, char const * new_unit, char const * new_category) {setType(new_type); setOwner(new_owner); setUnit(new_unit); setCategory(new_category);}

private:
    char m_type [MaxTypeLen + 1] = "";
    char m_owner [MaxOwnerLen + 1] = "";
    char m_unit [MaxUnitLen + 1] = "";
    char m_category [MaxCategoryLen + 1] = "";
};

template <int TicksPerSecond, int DaysToTrack = 7, typename SampleType = double, typename AggregateType = double, typename CountType = unsigned long long>
class Track {
public:
    using TimelineType = Timeline<TicksPerSecond, DaysToTrack, SampleType, AggregateType, CountType>;

public:
    Metadata const & metadata () const {return m_metadata;}
    TimelineType const & timeline () const {return m_timeline;}

    void setMetadata (char const * type_name, char const * owner_name, char const * unit_name, char const * category_name) {m_metadata.setAll(type_name, owner_name, unit_name, category_name);}
    void addSample (SampleType sample) {m_timeline.addSample(sample);}
    void tick () {m_timeline.tick();}

private:
    Metadata m_metadata;
    TimelineType m_timeline;
};

    }   // namespace Stats
}   // namespace y
