#ifndef ASN1_PROTO_CONVERTER_H_
#define ASN1_PROTO_CONVERTER_H_

#include <sstream>
#include <string>
#include <iostream>
#include <stack>

#include "asn1.pb.h"

namespace asn1_proto {
  class ASN1ProtoConverter {
   public:
    std::string ProtoToDER(const ASN1Object& asn1Obj);

   private:
    std::stringstream data_;
    std::vector<uint8_t> encoder;

    void ParseToHex(std::vector<uint8_t> encoder);
    std::vector<uint8_t> ParseObject(const ASN1Object& asn1Obj);
    std::vector<uint8_t> ParseSequence(const ASN1Seq& asn1Seq);
    std::vector<uint8_t> ParseInt(const ASN1Integer& asn1Int);
    std::vector<uint8_t> ParseNull(const ASN1Null& asn1Null);
    std::vector<uint8_t> AppendLengthPrimitive(uint64_t length);
    std::vector<uint8_t> AppendLengthConstructive(std::vector<std::vector<uint8_t>> len, int lengthPos);
    std::vector<uint8_t> AddNumbers(std::vector<uint8_t> len1, std::vector<uint8_t> len2);
  };
}

#endif
