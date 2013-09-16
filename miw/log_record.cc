/**
 * Copyright (c) 2013 XPLR Software Inc.
 * All rights reserved.
 * Author: Emmanuel Benazera <emmanuel.benazera@xplr.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of XPLR Software Inc nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY XPLR SOFTWARE INC ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XPLR SOFTWARE INC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "log_record.h"
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <iostream>

namespace miw
{

  log_record::log_record(const std::string &key,
			 const logdef &ld)
    :_key(key),_sum(1),_ld(ld)
  {
  }

  log_record::~log_record()
  {
  }

  std::string log_record::key() const
  {
    return _key;
  }

  void log_record::aggregation_union(const int &i,
				     const field &f)
  {
    std::string ftype = f.type();
    if (ftype == "int")
      {
	int_field *ifi = _ld.fields(i).mutable_int_fi();
	for (int j=0;j<f.int_fi().int_reap_size();j++)
	  ifi->add_int_reap(f.int_fi().int_reap(j));
      }
    else if (ftype == "string")
      {
	string_field *ifs = _ld.fields(i).mutable_str_fi();
	
	std::unordered_set<std::string> uno;
	for (int i=0;i<ifs->str_reap_size();i++)
	  uno.insert(ifs->str_reap(i));
	std::unordered_set<std::string>::const_iterator sit;
	
	for (int j=0;j<f.str_fi().str_reap_size();j++)
	  {
	    std::string str = f.str_fi().str_reap(j);
	    if ((sit=uno.find(str))==uno.end())
	      ifs->add_str_reap(str);
	  }
      }
    else if (ftype == "bool")
      {
	bool_field *ifb = _ld.fields(i).mutable_bool_fi();
	for (int j=0;j<f.bool_fi().bool_reap_size();j++)
	  ifb->add_bool_reap(f.bool_fi().bool_reap(j));
      }
    else if (ftype == "float")
      {
	float_field *iff = _ld.fields(i).mutable_real_fi();
	for (int j=0;j<f.real_fi().float_reap_size();j++)
	  iff->add_float_reap(f.real_fi().float_reap(j));
      }
  }

  void log_record::aggregation_sum(const int &i,
				   const field &f)
  {
    std::string ftype = f.type();
    if (ftype == "int")
      {
	int_field ifi = _ld.fields(i).int_fi();
	for (int j=0;j<f.int_fi().int_reap_size();j++)
	  ifi.set_int_reap(0,ifi.int_reap(0) + f.int_fi().int_reap(j));
      }
    else if (ftype == "float")
      {
	float_field iff = _ld.fields(i).real_fi();
	for (int j=0;j<f.real_fi().float_reap_size();j++)
	  iff.set_float_reap(0,iff.float_reap(0) + f.real_fi().float_reap(j));
      }
    else
      {
	std::cerr << "[Error]: trying to sum up non numerical field\n";
      }
  }

  void log_record::aggregation_max(const int &i,
				   const field &f)
  {
    std::string ftype = f.type();
    if (ftype == "int")
      {
	int_field ifi = _ld.fields(i).int_fi();
	for (int j=0;j<f.int_fi().int_reap_size();j++)
	  ifi.set_int_reap(0,std::max(ifi.int_reap(0),f.int_fi().int_reap(j)));
      }
    else if (ftype == "float")
      {
	float_field iff = _ld.fields(i).real_fi();
	for (int j=0;j<f.real_fi().float_reap_size();j++)
	  iff.set_float_reap(0,std::max(iff.float_reap(0),f.real_fi().float_reap(j)));
      }
    else
      {
	std::cerr << "[Error]: trying max operator on non numerical field\n";
      }
  }
  
  void log_record::aggregation_count(const int &i,
				     const field &f)
  {
    _ld.fields(i).set_count(f.count() + 1);
  }
  
  void log_record::merge(log_record *lr)
  {
    //std::cerr << "[Debug]: merging log records\n";
    
    if (!lr)
      return;
    
    // look for key fields for each record.
    // iterate remaining fields:
    // if 'aggregated', aggregate (e.g. sum, mean, union, ...)
    // else if not stored, skip (remove ?) field
    for (int i=0;i<lr->_ld.fields_size();i++)
      {
	if (!lr->_ld.fields(i).key())
	  {
	    if (lr->_ld.fields(i).aggregated())
	      {
		// aggregate into hosting record (this).
		std::string aggregation = lr->_ld.fields(i).aggregation();
		if (aggregation == "count")
		  {
		    aggregation_count(i,lr->_ld.fields(i));
		  }
		if (aggregation =="union")
		  {
		    aggregation_union(i,lr->_ld.fields(i));
		  }
		else if (aggregation == "sum")
		  {
		    aggregation_sum(i,lr->_ld.fields(i));
		  }
		else if (aggregation == "max")
		  {
		    aggregation_max(i,lr->_ld.fields(i));
		  }
	      }
	  }
      }
    _sum += lr->_sum;

    // merge original content.
    if (!_lines.empty() || !lr->_lines.empty())
      {
	std::vector<std::string> nlines;
	std::sort(_lines.begin(),_lines.end());
	std::sort(lr->_lines.begin(),lr->_lines.end());
	std::set_union(_lines.begin(),_lines.end(),lr->_lines.begin(),lr->_lines.end(),
		       std::inserter(nlines,nlines.begin()));
	_lines = nlines;
      }
  }

  void log_record::to_json(const field &f, Json::Value &jrec)
  {
    std::string ftype = f.type();
    Json::Value jsf;
    std::string json_fname = f.name();
    if (ftype == "int")
      {
	json_fname += "_i";
	int_field *ifi = f.mutable_int_fi();
	if (ifi->int_reap_size() > 1)
	  {
	    json_fname += "s";
	    for (int i=0;i<ifi->int_reap_size();i++)
	      jsf.append(ifi->int_reap(i));
	  }
	else if (ifi->int_reap_size() == 1)
	  jsf = ifi->int_reap(0);
      }
    else if (ftype == "string")
      {
	json_fname += "_s";
	string_field *ifs = f.mutable_str_fi();
	if (ifs->str_reap_size() > 1)
	  {
	    json_fname += "s";
	    for (int i=0;i<ifs->str_reap_size();i++)
	      jsf.append(ifs->str_reap(i));
	  }
	else if (ifs->str_reap_size() == 1)
	  jsf = ifs->str_reap(0);
      }
    else if (ftype == "bool")
      {
	json_fname += "_b";
	bool_field *ifb = f.mutable_bool_fi();
	if (ifb->bool_reap_size() > 1)
	  {
	    json_fname += "s";
	    for (int i=0;i<ifb->bool_reap_size();i++)
	      jsf.append(ifb->bool_reap(i));
	  }
	else if (ifb->bool_reap_size() == 1)
	  jsf = ifb->bool_reap(0);
      }
    else if (ftype == "float")
      {
	json_fname += "_f";
	float_field *iff = f.mutable_real_fi();
	if (iff->float_reap_size() > 1)
	  {
	    json_fname += "_fs";
	    for (int i=0;i<iff->float_reap_size();i++)
	      jsf.append(iff->float_reap(0));
	  }
	else if (iff->float_reap_size() == 1)
	  jsf = iff->float_reap(0);
      }
    if (!jsf.isNull())
      {
	if (f.aggregated())
	  {
	    if (f.aggregation() == "union")
	      jrec[json_fname]["add"] = jsf;
	    else if (f.aggregation() == "sum"
		     || f.aggregation() == "count")
	      jrec[json_fname]["inc"] = jsf;
	  }
	else jrec[json_fname] = jsf;
      }
    if (f.count() > 0)
      jrec["count"] = f.count();
  }

  Json::Value log_record::to_json() const
  {
    Json::Value jlrec;
    
    //debug
    //std::cerr << "number of fields: " << _ld.fields_size() << std::endl;
    //debug

    jlrec["id"] = _key;
    for  (int i=0;i<_ld.fields_size();i++)
      {
	field f = _ld.fields(i);
	log_record::to_json(f,jlrec);
      }

    // concatenate original log lines if any.
    if (!_lines.empty())
      {
	std::stringstream sst;
	std::for_each(_lines.begin(),_lines.end(),[&sst](const std::string &s){ sst << s << std::endl; });
	jlrec["content"]["add"] = sst.str();
      }
    
    if (!_ld.appname().empty())
      jlrec["appname"] = _ld.appname();
    jlrec["logs"]["inc"] = (int)_sum;

    //debug
    /*Json::FastWriter writer;
      std::cout << "JSON: " << writer.write(jlrec) << std::endl;*/
    //debug
    
    return jlrec;
  }
  
}
