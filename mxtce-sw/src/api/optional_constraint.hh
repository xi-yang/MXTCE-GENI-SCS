/*
 * optional_constraint.hh
 *
 *  Created on: Jul 19, 2011
 *      Author:
 */

#ifndef _OPTIONAL_CONSTRAINT_HH_
#define _OPTIONAL_CONSTRAINT_HH_

#include "api_constraint.hh"

class Apimsg_opti_constraint:public Apimsg_constraint
{
protected:

	string optional_constraint_type;

public:

	Apimsg_opti_constraint()
	{
		this->optional_constraint_type = "";
	}

	void setOptionalConsType(string t_optional_constraint_type){this->optional_constraint_type = t_optional_constraint_type;}
	string& getOptionalConsType() {return this->optional_constraint_type;}




};


#endif /* OPTIONAL_CONSTRAINT_HH_ */
