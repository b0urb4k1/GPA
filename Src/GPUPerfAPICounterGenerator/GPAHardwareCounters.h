//==============================================================================
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Maintains a set of hardware counters
//==============================================================================

#ifndef _GPA_HARDWARE_COUNTERS_H_
#define _GPA_HARDWARE_COUNTERS_H_

#include "GPAInternalCounter.h"

/// Struct to describe a hardware counter
struct GPA_HardwareCounterDescExt
{
    gpa_uint32 m_groupIndex;                     ///< index of group containing this counter
    gpa_uint32 m_groupIdDriver;                  ///< group ID according to the driver
    gpa_uint32 m_counterIdDriver;                ///< counter ID according to the driver
    GPA_HardwareCounterDesc* m_pHardwareCounter; ///< the internal counter
};

/// Maintains a set of hardware counters
class GPA_HardwareCounters
{
public:
    /// Initializes an instance of the GPA_HardwareCounters class.
    GPA_HardwareCounters()
    {
        Clear();
    }

    /// Destructor
    virtual ~GPA_HardwareCounters()
    {
    }

    /// Clears all counter data
    void Clear()
    {
        m_currentGroupUsedCounts.clear();
        m_counters.clear();
        m_ppCounterGroupArray = nullptr;
        m_pGroups = nullptr;
        m_groupCount = 0;
        m_pAdditionalGroups = nullptr;
        m_additionalGroupCount = 0;
        m_pSQCounterGroups = nullptr;
        m_sqGroupCount = 0;
        m_gpuTimestampIndex = static_cast<unsigned int>(-1);
        m_gpuTimeIndex = static_cast<unsigned int>(-1);
        m_gpuTimestampPreBottomCounterIndex = static_cast<unsigned int>(-1);
        m_gpuTimestampPostBottomCounterIndex = static_cast<unsigned int>(-1);
        m_gpuTimestampTopCounterIndex = static_cast<unsigned int>(-1);
        m_gpuTimeBottomToBottomCounterIndex = static_cast<unsigned int>(-1);
        m_gpuTimeTopToBottomCounterIndex = static_cast<unsigned int>(-1);
        m_countersGenerated = false;
    }

    /// Obtains the number of hardware counters
    /// \return the number of hardware counters
    gpa_uint32 GetNumCounters()
    {
        return (gpa_uint32)m_counters.size();
    }

    /// Gets the name of the specified counter
    /// \param index the index of the counter whose name is needed
    /// \return the name of the specified counter
    const char* GetCounterName(gpa_uint32 index)
    {
        return m_counters[index].m_pHardwareCounter->m_pName;
    }

    /// Gets the description of the specified counter
    /// \param index the index of the counter whose description is needed
    /// \return the description of the specified counter
    const char* GetCounterDescription(gpa_uint32 index)
    {
        return m_counters[index].m_pHardwareCounter->m_pDescription;
    }

    /// List of counter groups as defined by the list of internal counters in each group.
    GPA_HardwareCounterDesc** m_ppCounterGroupArray;

    /// List of internal counter groups
    GPA_CounterGroupDesc* m_pGroups;

    /// List of internal counter groups exposed by the driver, but not known by GPA
    GPA_CounterGroupDesc* m_pAdditionalGroups;

    /// The number of internal counter groups
    unsigned int m_groupCount;

    /// The number of internal counter groups exposed by the driver, but not known by GPA
    unsigned int m_additionalGroupCount;

    /// List of SQCounterGroupDesc
    GPA_SQCounterGroupDesc* m_pSQCounterGroups;

    /// The number of internal SQ counter groups
    unsigned int m_sqGroupCount;

    unsigned int m_gpuTimestampIndex; ///< Index of the GPUTimestamp group (-1 if it doesn't exist)
    unsigned int m_gpuTimeIndex;      ///< Index of the GPUTime group (-1 if it doesn't exist)

    unsigned int m_gpuTimestampPreBottomCounterIndex;  ///< the index of the GPU Timestamp pre-event Bottom counter (-1 if it doesn't exist)
    unsigned int m_gpuTimestampPostBottomCounterIndex; ///< the index of the GPU Timestamp post-event Bottom counter (-1 if it doesn't exist)
    unsigned int m_gpuTimestampTopCounterIndex;        ///< the index of the GPU Timestamp pre-event Top counter (-1 if it doesn't exist)
    unsigned int m_gpuTimeBottomToBottomCounterIndex;  ///< the index of the GPUTime Bottom-to-Bottom counter (-1 if it doesn't exist)
    unsigned int m_gpuTimeTopToBottomCounterIndex;     ///< the index of the GPUTime Top-to-Bottom counter (-1 if it doesn't exist)

    /// indicates that the internal counters have been generated
    bool m_countersGenerated;

    /// vector of hardware counters
    std::vector<GPA_HardwareCounterDescExt> m_counters;

    /// List of the number of counters which have been enabled in each group
    std::vector<int> m_currentGroupUsedCounts;
};

#endif //_GPA_HARDWARE_COUNTERS_H_
