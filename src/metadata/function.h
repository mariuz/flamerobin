/*
  Copyright (c) 2004-2016 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#ifndef FR_FUNCTION_H
#define FR_FUNCTION_H

#include "metadata/collection.h"
#include "metadata/privilege.h"


class ProgressIndicator;

class Function: public MetadataItem
{
private:
    wxString libraryNameM, entryPointM, definitionM, retstrM, paramListM;
	ParameterPtrs parametersM;
	bool legacyFunctionM;
protected:
    virtual void loadProperties();

	virtual void loadChildren();
	virtual void lockChildren();
	virtual void unlockChildren();

public:
    Function(DatabasePtr database, const wxString& name);

	bool getChildren(std::vector<MetadataItem *>& temp);


	ParameterPtrs::iterator begin();
	ParameterPtrs::iterator end();
	ParameterPtrs::const_iterator begin() const;
	ParameterPtrs::const_iterator end() const;

	size_t getParamCount() const;
	ParameterPtr findParameter(const wxString& name) const;

    virtual const wxString getTypeName() const;
    virtual wxString getDropSqlStatement() const;
    wxString getCreateSql();
    wxString getDefinition();
	bool getLegacyFunction();
	wxString getLibraryName();
    wxString getEntryPoint();
	wxString getSource();


    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

class FunctionLegacy : public Function
{
private:
	wxString libraryNameM, entryPointM, definitionM, retstrM, paramListM;
protected:
	virtual void loadProperties();
public:
	FunctionLegacy(DatabasePtr database, const wxString& name);
	virtual const wxString getTypeName() const;
	virtual wxString getDropSqlStatement() const;
	wxString getCreateSql();
	wxString getDefinition();
	wxString getLibraryName();
	wxString getEntryPoint();
	virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

class FunctionSQL : public Function
{
private:
	std::vector<Privilege> privilegesM;
	ParameterPtrs parametersM;
protected:
	virtual void loadChildren();
	virtual void lockChildren();
	virtual void unlockChildren();
public:
	FunctionSQL(DatabasePtr database, const wxString& name);

	bool getChildren(std::vector<MetadataItem *>& temp);


	ParameterPtrs::iterator begin();
	ParameterPtrs::iterator end();
	ParameterPtrs::const_iterator begin() const;
	ParameterPtrs::const_iterator end() const;

	size_t getParamCount() const;
	ParameterPtr findParameter(const wxString& name) const;

	wxString getOwner();
	wxString getSource();
	wxString getAlterSql(bool full = true);
	wxString getDefinition();   // used for calltip in sql editor
	std::vector<Privilege>* getPrivileges();

	void checkDependentProcedures();

	virtual const wxString getTypeName() const;
	virtual void acceptVisitor(MetadataItemVisitor* visitor);

};

class Functions: public MetadataCollection<Function>
{
protected:
    virtual void loadChildren();
public:
    Functions(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

#endif
