#pragma once
#include "fusion_includes.h"
#include "util/stat_log_impl.h"
//using namespace boost::fusion;

#define NAME static constexpr const char* name
namespace rc
{
namespace stat_log
{


template <int N, typename T>
struct StatTable
{

};

//Operational proxies
struct OpProxyBasic
{
   int* serial_ptr = nullptr;
   void setShmPtr(void* ptr)
   {
      serial_ptr = reinterpret_cast<int*>(ptr);
   }

   void write(int i)
   {
      (*serial_ptr) += i;
   }

   //TODO: temporary
   auto& getValue()
   {
      return (*serial_ptr);
   }
};


//Observer/Control proxies
//TODO:
//

template<typename Tag, typename T>
auto getStatHandleView(T& stats)
{
   return boost::fusion::filter_view<T, detail::matches_tag<Tag>>(stats);
}

template <typename Tag, typename T>
auto& getValue(T& stats)
{
   using namespace boost::fusion;
   auto statHdlView = getStatHandleView<Tag>(stats);
   static_assert(result_of::size<decltype(statHdlView)>::value == 1,
         "getValues requires a Leaf Tag!");
    return deref(begin(statHdlView)).getValue();
}

template <typename UserStatH>
struct LogStatOperational : detail::LogStatBase<UserStatH, true>
{
};

template <typename UserStatH>
struct LogStatControl : detail::LogStatBase<UserStatH, false>
{
};

}
}



//MAKE_STAT_TAGS is a convenience wrapper for creating statistics.
//For example:
//
//     struct RxCountTag{
//        NAME = "RX_COUNTERS";
//     };
//     struct CorrScoreTag{
//        NAME = "CORR_SCORE";
//     };
//     struct LinkIndTag{
//        NAME = "LINK_IND";
//     };
//
//     using ChildTypes = vector<RxCountTag, CorrScoreTag, LinkIndTag>;
//
//Can be replaced by a single line:
//     MAKE_STAT_TAGS( (RxCount) (CorrScore) (LinkInd))
//
//TODO: it would be really cool if we can define stat hierarchies in this
// way. i.e.
//    MAKE_STAT_TAGS( (Level1Stats (RxCount) (CorrScore) (LinkInd))
//                    (Level2Stats (IpDown) (IpUp))
//                   )
//
//But it seems that boost's preprocessor library cannot handle
// nested sequences:
// http://stackoverflow.com/questions/26739178/are-two-dimensional-sequences-possible-with-boost-preprocessor
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/transform.hpp>

#define _MAKE_STAT_TAG(r, data, elem) BOOST_PP_CAT(elem,data)

#define _MAKE_STAT_TAG_STRUCT(r, data, elem) \
   struct _MAKE_STAT_TAG(r, data, elem) { \
         NAME = BOOST_PP_STRINGIZE(elem); \
            };

#if 1
#define MAKE_STAT_TAGS(args) \
      BOOST_PP_SEQ_FOR_EACH (_MAKE_STAT_TAG_STRUCT, Tag, args) \
   using ChildTypes = boost::fusion::vector<BOOST_PP_SEQ_ENUM(\
         BOOST_PP_SEQ_TRANSFORM(_MAKE_STAT_TAG, Tag, args))>;
#else
#endif

