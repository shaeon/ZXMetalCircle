#pragma once
#include <thread>
class CMemorySystem { public: static CMemorySystem *Get () { return 0; } };
class CMultiCoreSupport { public: CMultiCoreSupport (CMemorySystem *) {} virtual ~CMultiCoreSupport () {}
  bool Initialize () { m_T = std::thread ([this]{ Run (1); }); m_T.detach (); return true; }
  virtual void Run (unsigned) = 0; std::thread m_T; };
