//==============================================================================
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Manages a set of public counters
//==============================================================================


#include "GPAPublicCounters.h"
#include <stdio.h>
#include <sstream>
#include <string.h> // for strcpy
#include "Utility.h"
#include "Logging.h"

GPA_PublicCounter::GPA_PublicCounter(unsigned int index, const char* pName, const char* pDescription, GPA_Type dataType, GPA_Usage_Type usageType, GPA_CounterType counterType, vector< gpa_uint32 >& internalCountersRequired, const char* pComputeExpression)
{
    m_index = index;
    m_pName = pName;
    m_pDescription = pDescription;
    m_dataType = dataType;
    m_usageType = usageType;
    m_counterType = counterType;
    m_internalCountersRequired = internalCountersRequired;
    m_pComputeExpression = pComputeExpression;
}


//------------------------------------------------------------------------------------------------------------------------------------------------------------

void GPA_PublicCounters::DefinePublicCounter(const char* pName, const char* pDescription, GPA_Type dataType, GPA_Usage_Type usageType, GPA_CounterType counterType, vector< gpa_uint32 >& internalCountersRequired, const char* pComputeExpression)
{
    assert(pName);
    assert(pDescription);
    assert(dataType < GPA_TYPE__LAST);
    assert(counterType < GPA_COUNTER_TYPE__LAST);
    assert(internalCountersRequired.size() > 0);
    assert(pComputeExpression);
    assert(strlen(pComputeExpression) > 0);

    unsigned int index = (unsigned int)m_counters.size();

    m_counters.push_back(GPA_PublicCounter(index, pName, pDescription, dataType, usageType, counterType, internalCountersRequired, pComputeExpression));
}


void GPA_PublicCounters::Clear()
{
    m_counters.clear();
    m_countersGenerated = false;
}


gpa_uint32 GPA_PublicCounters::GetNumCounters()
{
    return (gpa_uint32) m_counters.size();
}


/// Evaluates a counter formula expression
/// T is public counter type
/// \param pExpression the counter formula
/// \param[out] pResult the result value
/// \param results list of the hardware counter results
/// \param internalCounterTypes list of the hardware counter types
/// \param resultType the coutner result type
/// \param pHwInfo the hardware info
template<class T, class InternalCounterType>
static void EvaluateExpression(const char* pExpression, void* pResult, vector< char* >& results, vector< GPA_Type >& internalCounterTypes, GPA_Type resultType,
                               const GPA_HWInfo* pHwInfo)
{
    internalCounterTypes;

    assert(nullptr != pHwInfo);

    size_t expressionLen = strlen(pExpression) + 1;
    char* pBuf = new(std::nothrow) char[expressionLen];

    strcpy_s(pBuf, expressionLen, pExpression);

    vector< T > stack;
    T* pWriteResult = (T*)pResult;

    char* pContext;
    char* pch = strtok_s(pBuf, " ,", &pContext);

    while (nullptr != pch)
    {
        if (*pch == '*')
        {
            assert(stack.size() >= 2);

            T p2 = stack.back();
            stack.pop_back();
            T p1 = stack.back();
            stack.pop_back();
            stack.push_back(p1 * p2);
        }
        else if (*pch == '/')
        {
            assert(stack.size() >= 2);

            T p2 = stack.back();
            stack.pop_back();
            T p1 = stack.back();
            stack.pop_back();

            if (p2 != (T)0)
            {
                stack.push_back(p1 / p2);
            }
            else
            {
                stack.push_back((T)0);
            }
        }
        else if (*pch == '+')
        {
            assert(stack.size() >= 2);

            T p2 = stack.back();
            stack.pop_back();
            T p1 = stack.back();
            stack.pop_back();
            stack.push_back(p1 + p2);
        }
        else if (*pch == '-')
        {
            assert(stack.size() >= 2);

            T p2 = stack.back();
            stack.pop_back();
            T p1 = stack.back();
            stack.pop_back();
            stack.push_back(p1 - p2);
        }
        else if (*pch == '(')
        {
            // constant
            T constant = (T)0;
            int scanResult = 0;

            if (resultType == GPA_TYPE_FLOAT32)
            {
#ifdef _LINUX
                scanResult = sscanf(pch, "(%f)", (gpa_float32*)&constant);
#else
                scanResult = sscanf_s(pch, "(%f)", (gpa_float32*)&constant);
#endif // _LINUX
            }
            else if (resultType == GPA_TYPE_FLOAT64)
            {
#ifdef _LINUX
                scanResult = sscanf(pch, "(%lf)", (gpa_float64*)&constant);
#else
                scanResult = sscanf_s(pch, "(%lf)", (gpa_float64*)&constant);
#endif // _LINUX
            }
            else if (resultType == GPA_TYPE_UINT32)
            {
#ifdef _LINUX
                scanResult = sscanf(pch, "(%u)", (gpa_uint32*)&constant);
#else
                scanResult = sscanf_s(pch, "(%u)", (gpa_uint32*)&constant);
#endif // _LINUX
            }
            else if (resultType == GPA_TYPE_UINT64)
            {
#ifdef _LINUX
                scanResult = sscanf(pch, "(%llu)", (gpa_uint64*)&constant);
#else
                scanResult = sscanf_s(pch, "(%I64u)", (gpa_uint64*)&constant);
#endif // _LINUX
            }
            else
            {
                // Unsupported public counter type
                assert(false);
            }

            assert(scanResult == 1);

            stack.push_back(constant);
        }
        else if (_strcmpi(pch, "num_shader_engines") == 0)
        {
            stack.push_back((T)pHwInfo->GetNumberShaderEngines());
        }
        else if (_strcmpi(pch, "num_simds") == 0)
        {
            stack.push_back((T)pHwInfo->GetNumberSIMDs());
        }
        else if (_strcmpi(pch, "su_clocks_prim") == 0)
        {
            stack.push_back((T)pHwInfo->GetSUClocksPrim());
        }
        else if (_strcmpi(pch, "num_prim_pipes") == 0)
        {
            stack.push_back((T)pHwInfo->GetNumberPrimPipes());
        }
        else if (_strcmpi(pch, "TS_FREQ") == 0)
        {
            stack.push_back((T)pHwInfo->GetTimeStampFrequency());
        }
        else if (_strcmpi(pch, "max") == 0)
        {
            assert(stack.size() >= 2);

            T p2 = stack.back();
            stack.pop_back();
            T p1 = stack.back();
            stack.pop_back();

            if (p1 > p2)
            {
                stack.push_back(p1);
            }
            else
            {
                stack.push_back(p2);
            }
        }
        else if (_strcmpi(pch, "max16") == 0)
        {
            assert(stack.size() >= 16);

            // initialize the max value to the 1st item
            T maxValue = stack.back();
            stack.pop_back();

            // pop the last 15 items and compute the max
            for (int i = 0; i < 15; i++)
            {
                T value = stack.back();
                stack.pop_back();

                maxValue = (maxValue > value) ? maxValue : value;
            }

            stack.push_back(maxValue);
        }
        else if (_strcmpi(pch, "max32") == 0)
        {
            assert(stack.size() >= 32);

            // initialize the max value to the 1st item
            T maxValue = stack.back();
            stack.pop_back();

            // pop the last 31 items and compute the max
            for (int i = 0; i < 31; i++)
            {
                T value = stack.back();
                stack.pop_back();

                maxValue = (maxValue > value) ? maxValue : value;
            }

            stack.push_back(maxValue);
        }
        else if (_strcmpi(pch, "max44") == 0)
        {
            assert(stack.size() >= 44);

            // initialize the max value to the 1st item
            T maxValue = stack.back();
            stack.pop_back();

            // pop the last 43 items and compute the max
            for (int i = 0; i < 43; i++)
            {
                T value = stack.back();
                stack.pop_back();

                maxValue = (maxValue > value) ? maxValue : value;
            }

            stack.push_back(maxValue);
        }
        else if (_strcmpi(pch, "max64") == 0)
        {
            assert(stack.size() >= 64);

            // initialize the max value to the 1st item
            T maxValue = stack.back();
            stack.pop_back();

            // pop the last 63 items and compute the max
            for (int i = 0; i < 63; i++)
            {
                T value = stack.back();
                stack.pop_back();

                maxValue = (maxValue > value) ? maxValue : value;
            }

            stack.push_back(maxValue);
        }
        else if (_strcmpi(pch, "min") == 0)
        {
            assert(stack.size() >= 2);

            T p2 = stack.back();
            stack.pop_back();
            T p1 = stack.back();
            stack.pop_back();

            if (p1 < p2)
            {
                stack.push_back(p1);
            }
            else
            {
                stack.push_back(p2);
            }
        }
        else if (_strcmpi(pch, "ifnotzero") == 0)
        {
            assert(stack.size() >= 3);

            T condition = stack.back();
            stack.pop_back();
            T resultTrue = stack.back();
            stack.pop_back();
            T resultFalse = stack.back();
            stack.pop_back();

            if (condition != 0)
            {
                stack.push_back(resultTrue);
            }
            else
            {
                stack.push_back(resultFalse);
            }
        }
        else if (_strcmpi(pch, "sum4") == 0)
        {
            assert(stack.size() >= 4);
            T sum = 0;

            // pop the last 4 items and add them together
            for (int i = 0; i < 4; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else if (_strcmpi(pch, "sum8") == 0)
        {
            assert(stack.size() >= 8);
            T sum = 0;

            // pop the last 8 items and add them together
            for (int i = 0; i < 8; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else if (_strcmpi(pch, "sum10") == 0)
        {
            assert(stack.size() >= 10);
            T sum = 0;

            // pop the last 10 items and add them together
            for (int i = 0; i < 10; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else if (_strcmpi(pch, "sum11") == 0)
        {
            assert(stack.size() >= 11);
            T sum = 0;

            // pop the last 11 items and add them together
            for (int i = 0; i < 11; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else if (_strcmpi(pch, "sum12") == 0)
        {
            assert(stack.size() >= 12);
            T sum = 0;

            // pop the last 12 items and add them together
            for (int i = 0; i < 12; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else if (_strcmpi(pch, "sum16") == 0)
        {
            assert(stack.size() >= 16);
            T sum = 0;

            // pop the last 16 items and add them together
            for (int i = 0; i < 16; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else if (_strcmpi(pch, "sum32") == 0)
        {
            assert(stack.size() >= 32);
            T sum = 0;

            // pop the last 32 items and add them together
            for (int i = 0; i < 32; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else if (_strcmpi(pch, "sum44") == 0)
        {
            assert(stack.size() >= 44);
            T sum = 0;

            // pop the last 44 items and add them together
            for (int i = 0; i < 44; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else if (_strcmpi(pch, "sum64") == 0)
        {
            assert(stack.size() >= 64);
            T sum = 0;

            // pop the last 64 items and add them together
            for (int i = 0; i < 64; i++)
            {
                T value = stack.back();
                stack.pop_back();

                sum += value;
            }

            stack.push_back(sum);
        }
        else
        {
            // must be number, reference to internal counter
            gpa_uint32 index;
#ifdef _LINUX
            int scanResult = sscanf(pch, "%d", &index);
#else
            int scanResult = sscanf_s(pch, "%d", &index);
#endif
            UNREFERENCED_PARAMETER(scanResult);
            assert(scanResult == 1);

            assert(index < results.size());

            if (index < results.size())
            {
                InternalCounterType internalVal = *((InternalCounterType*)results[index]);
                T internalValFloat = (T)internalVal;
                stack.push_back(internalValFloat);
            }
            else
            {
                // the index was invalid, so the counter result is unknown
                assert(!"counter index in equation is out of range");
                stack.push_back(0);
            }
        }

        pch = strtok_s(nullptr, " ,", &pContext);
    }

    if (stack.size() != 1)
    {
        std::stringstream ss;
        ss << "Invalid formula: " << pExpression << ".";
        GPA_LogError(ss.str().c_str());
    }

    assert(stack.size() == 1);
    *pWriteResult = stack.back();

    delete[] pBuf;
}

void GPA_PublicCounters::ComputeCounterValue(gpa_uint32 counterIndex, vector< char* >& results, vector< GPA_Type >& internalCounterTypes, void* pResult, GPA_HWInfo* pHwInfo)
{
    if (nullptr != m_counters[counterIndex].m_pComputeExpression)
    {
#ifdef AMDT_INTERNAL
        GPA_LogDebugCounterDefs("'%s' equation is %s", m_counters[counterIndex].m_pName, m_counters[counterIndex].m_pComputeExpression);
#endif

        if (internalCounterTypes[0] == GPA_TYPE_UINT64)
        {
            if (m_counters[counterIndex].m_dataType == GPA_TYPE_FLOAT32)
            {
                EvaluateExpression<gpa_float32, gpa_uint64>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_FLOAT64)
            {
                EvaluateExpression<gpa_float64, gpa_uint64>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_UINT32)
            {
                EvaluateExpression<gpa_uint32, gpa_uint64>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_UINT64)
            {
                EvaluateExpression<gpa_uint64, gpa_uint64>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_INT32)
            {
                EvaluateExpression<gpa_int32, gpa_uint64>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_INT64)
            {
                EvaluateExpression<gpa_int64, gpa_uint64>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else
            {
                // public counter type not recognized or not currently supported.
                assert(false);
            }
        }
        else if (internalCounterTypes[0] == GPA_TYPE_UINT32)
        {
            if (m_counters[counterIndex].m_dataType == GPA_TYPE_FLOAT32)
            {
                EvaluateExpression<gpa_float32, gpa_uint32>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_FLOAT64)
            {
                EvaluateExpression<gpa_float64, gpa_uint32>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_UINT32)
            {
                EvaluateExpression<gpa_uint32, gpa_uint32>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_UINT64)
            {
                EvaluateExpression<gpa_uint64, gpa_uint32>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_INT32)
            {
                EvaluateExpression<gpa_int32, gpa_uint32>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else if (m_counters[counterIndex].m_dataType == GPA_TYPE_INT64)
            {
                EvaluateExpression<gpa_int64, gpa_uint32>(m_counters[counterIndex].m_pComputeExpression, pResult, results, internalCounterTypes, m_counters[counterIndex].m_dataType, pHwInfo);
            }
            else
            {
                // public counter type not recognized or not currently supported.
                assert(false);
            }
        }
    }
    else
    {
        // no method of evaluation specified for counter
        assert(false);
    }
}


