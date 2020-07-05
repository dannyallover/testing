#include "asn1_proto_converter.h"
#include <typeinfo>
#include <string>
#include <vector>
#include <cstdint>
#include <stack>
#include <cmath>

#define OFFSET 2

namespace asn1_proto {

  std::vector<uint8_t> ASN1ProtoConverter::AddNumbers(std::vector<uint8_t> len1, std::vector<uint8_t> len2) {
    if(len1.size() > len2.size()) {
      len1.swap(len2);
    }

    std::vector<uint8_t> results;

    int length1 = len1.size();
    int length2 = len2.size();
    int diff = length2 - length1;

    uint16_t carry = 0;

    for(int i=length1-1; i>= 0; i--) {
      uint16_t num = len1[i] + len2[i+diff] + carry;
      results.push_back(num%0x100);
      carry = (num/0x100);
    }

    for(int i = length2-length1-1; i>=0; i--) {
      uint16_t num = len2[i] + carry;
      results.push_back(num%0x100);
      carry = (num/0x100);
    }

    if(carry) {
      results.push_back(carry);
    }

    reverse(results.begin(), results.end());

    return results;
  }

  std::vector<uint8_t> ASN1ProtoConverter::AppendLengthConstructive(std::vector<std::vector<uint8_t>> length, int lengthPos) {
    int i = 1;
    while(i < length.size()) {
      length[0] = AddNumbers(length[0], length[i]);
      i++;
    }
    encoder.insert(encoder.begin()+lengthPos, length[0].begin(), length[0].end());
    if(length[0].size() > 1) {
      uint8_t l = (1 << 7);
      l += length[0].size();
      std::cout << "the value of l is " << (l&0xFF) << std::endl;
      encoder.insert(encoder.begin()+lengthPos, l);
      uint8_t extra = length[0].size() - 1;
      length[0] = AddNumbers(length[0], {extra});
    } else if((length[0][0]&0xFF) > 127) {
      uint8_t l = (1 << 7);
      l += 1;
      std::cout << "the value of l is " << (l&0xFF) << std::endl;
      encoder.insert(encoder.begin()+lengthPos, l);
      uint8_t extra = 1;
      length[0] = AddNumbers(length[0], {extra});
    }
    return length[0];
  }

  std::vector<uint8_t> ASN1ProtoConverter::AppendLengthPrimitive(uint64_t length) {
    if(length <= 127) {
      encoder.push_back((length&0xFF));
      uint8_t l = (length&0xFF);
      return {l};
    } else {
      uint8_t l = 0;
      l = (l | (1 << 7));
      double byte = 0;
      for(int i = 0; i < 64; i++) {
        if(((length >> i) & 0x1) == 1) {
          byte = i;
        }
      }

      uint8_t numBytes = ceil((byte/0x08));
      l = l + numBytes;
      std::vector<uint8_t> ret;
      encoder.push_back((l&0xFF));
      for(int i = numBytes-1; i >=0; i--) {
        encoder.push_back((length>>(i*8))&0xFF);
        ret.push_back((length>>(i*8))&0xFF);
      }
      return AddNumbers(ret, {numBytes});
    }
  }

  std::vector<uint8_t> ASN1ProtoConverter::ParseNull(const ASN1Null& asn1Null) {
    encoder.push_back(0x05);
    encoder.push_back(0x00);
    return {0};
  }

  std::vector<uint8_t> ASN1ProtoConverter::ParseInt(const ASN1Integer& asn1Int) {
    encoder.push_back(0x02);
    std::string val = asn1Int.val().data();
    std::vector<uint8_t> len;
    if(val.begin() != val.end()) {
      len = AppendLengthPrimitive(val.size());
      encoder.insert(encoder.end(), val.begin(), val.end());
    } else {
      len = AppendLengthPrimitive(0x1);
      encoder.push_back(0x01);
    }
    return len;
  }

  std::vector<uint8_t> ASN1ProtoConverter::ParseSequence(const ASN1Seq& asn1Seq) {
    if(asn1Seq.asn1_obj_size() != 0) {
      encoder.push_back(0x30);
      int lengthPos = encoder.size();
      std::vector<std::vector<uint8_t>> len;
      for(const auto asn1Obj : asn1Seq.asn1_obj()) {
        std::vector<uint8_t> res = ParseObject(asn1Obj);
        len.push_back(res);
        std::cout << "here is what is in len again " << std::endl;
        for(auto x : len) {
          for(uint8_t byte : x) {
            std::cout << std::hex << ((byte >> 4) & 0xF);
            std::cout << (byte & 0xF);
            std::cout << " ";
          }
          std::cout << std::endl;
        }
      }
      return AppendLengthConstructive(len, lengthPos);
    } else {
      return ParseNull(asn1Seq.asn1_null());
    }
  }
 

  std::vector<uint8_t> ASN1ProtoConverter::ParseObject(const ASN1Object& asn1Obj) {
    std::vector<uint8_t> size;
    if(asn1Obj.has_asn1_int()) {
      size=ParseInt(asn1Obj.asn1_int());
    } else if(asn1Obj.has_asn1_seq()) {
      size=ParseSequence(asn1Obj.asn1_seq());
    } else {
      size=ParseNull(asn1Obj.asn1_null());
    }
    size = AddNumbers(size, {0x2});
    return size;
  }

  void ASN1ProtoConverter::ParseToHex(std::vector<uint8_t> encoder) {
    for(uint8_t byte : encoder) {
      data_ << std::hex << ((byte >> 4) & 0xF);
      data_ << (byte & 0xF);
      data_ << " ";
    }
  }

  std::string ASN1ProtoConverter::ProtoToDER(const ASN1Object& asn1Obj) {
    data_.clear();
    encoder.clear();
    ParseObject(asn1Obj);
    std::cout << "were in business" << std::endl;
    ParseToHex(encoder);
    return data_.str();
  }

}
