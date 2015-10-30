/*
 * NodeJS wrapper-plugin for Aqbanking
 * Copyright (C) 2015  Lukas Matt <lukas@zauberstuhl.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include <gwenhywfar/cgui.h>
#include <gwenhywfar/gui_be.h>

#include <aqbanking/banking.h>
#include <aqbanking/jobgetbalance.h>
#include <aqbanking/jobgettransactions.h>

#include "ab_helpers.h"
#include "ab_gui_interface.h"

using namespace std;

int UB::Helper::init(void) {
  this->gui = GWEN_Gui_CGui_new();
  this->ab = AB_Banking_new("unholy_banking", 0, AB_BANKING_EXTENSION_NONE);

  GWEN_Gui_SetGui(this->gui);

  int rv = AB_Banking_Init(ab);
  if (rv) {
    cout << "Could not initialize (AB_Banking_Init)." << endl;
    return 3;
  }

  rv = AB_Banking_OnlineInit(ab);
  if (rv) {
    cout << "Could not do online initialize (AB_Banking_OnlineInit)." << endl;
    return 3;
  }

  //GWEN_Gui_SetGetPasswordFn(this->gui, CUSTOM_GETPASSWORD_FN);
  // TODO fix me
  //GWEN_Gui_SetCheckCertFn(this->gui, CUSTOM_CHECKCERT_FN);
  return 0;
}

int UB::Helper::close(void) {
  int rv;
  rv = AB_Banking_OnlineFini(ab);
  if (rv) {
    cout << "Could not do online deinit. (AB_Banking_OnlineFini)." << endl;
    return 3;
  }

  rv = AB_Banking_Fini(ab);
  if (rv) {
    cout << "Could not do deinit. (AB_Banking_Fini)." << endl;
    return 3;
  }

  AB_Banking_free(ab);
  return 0;
}

int UB::Helper::list_accounts(void) {
  //int rv;
  AB_ACCOUNT_LIST2 *accs;
  //aqbanking_Account *account;

  accs = AB_Banking_GetAccounts(ab);
  if (accs) {
    AB_ACCOUNT_LIST2_ITERATOR *it;
    it=AB_Account_List2_First(accs);
    if (it) {
      AB_ACCOUNT *a;
      a=AB_Account_List2Iterator_Data(it);
      cout << "enter while:" << endl;
      while(a) {
        AB_PROVIDER *pro = AB_Account_GetProvider(a);
        //AB_Account_GetAccountNumber(a));
        //AB_Account_GetAccountName(a));
        //AB_Provider_GetName(pro));
        //AB_Account_GetBankCode(a));
        //AB_Account_GetBankName(a));
        cout << AB_Account_GetAccountNumber(a) << endl;
        cout << AB_Account_GetBankCode(a) << endl;

        a = AB_Account_List2Iterator_Next(it);
      }
      AB_Account_List2Iterator_free(it);
    }
    AB_Account_List2_free(accs);
  }
  return 0;
}

int UB::Helper::transactions(AB_ACCOUNT * a) {
  int rv;
  //double tmpDateTime = 0;
  //const char *bank_code;
  //const char *account_no;
  GWEN_TIME *gwTime;
  const char *dateFrom=NULL, *dateTo=NULL;
  //static char *kwlist[] = {"dateFrom", "dateTo", NULL};

  AB_JOB *job = 0;
  AB_JOB_LIST2 *jl = 0;
  AB_IMEXPORTER_CONTEXT *ctx = 0;
  AB_IMEXPORTER_ACCOUNTINFO *ai;

  job = AB_JobGetTransactions_new(a);
  if (dateFrom != NULL) {
    gwTime = GWEN_Time_fromString(dateFrom, "YYYYMMDD");
    AB_JobGetTransactions_SetFromTime(job, gwTime);
  }
  if (dateTo != NULL) {
    gwTime = GWEN_Time_fromString(dateTo, "YYYYMMDD");
    AB_JobGetTransactions_SetToTime(job, gwTime);
  }
  rv = AB_Job_CheckAvailability(job);
  if (rv) {
    //Transaction retrieval is not supported
    return 3;
  }

  jl = AB_Job_List2_new();
  AB_Job_List2_PushBack(jl, job);
  ctx = AB_ImExporterContext_new();
  rv = AB_Banking_ExecuteJobs(ab, jl, ctx);
  if (rv) {
    //Could not retrieve transactions
    return 3;
  }

  ai = AB_ImExporterContext_GetFirstAccountInfo (ctx);
  while(ai) {
    const AB_TRANSACTION * t;
    t = AB_ImExporterAccountInfo_GetFirstTransaction(ai);
    while(t) {
      const AB_VALUE *v;
      //AB_TRANSACTION_STATUS state;
      v=AB_Transaction_GetValue(t);
      if (v) {
        const GWEN_STRINGLIST *sl;
        const GWEN_TIME *tdtime;
        const char *purpose;
        sl = AB_Transaction_GetPurpose(t);
        if (sl) {
          purpose = GWEN_StringList_FirstString(sl);
          if (purpose == NULL) {
            purpose = "";
          }
        } else {
          purpose = "";
        }

        fprintf(stderr, "[%-10s/%-10s][%-10s/%-10s] %-32s (%.2f %s)\n",
          AB_Transaction_GetRemoteIban(t),
          AB_Transaction_GetRemoteBic(t),
          AB_Transaction_GetRemoteAccountNumber(t),
          AB_Transaction_GetRemoteBankCode(t),
          purpose,
          AB_Value_GetValueAsDouble(v),
          AB_Value_GetCurrency(v)
        );
        tdtime = AB_Transaction_GetDate(t);
        //GWEN_Time_Seconds(tdtime)
        //AB_Transaction_GetLocalAccountNumber(t)
        //AB_Transaction_GetLocalBankCode(t);
        //AB_Transaction_GetLocalIban(t);
        //AB_Transaction_GetLocalBic(t);
        //AB_Transaction_GetRemoteAccountNumber(t)
        //AB_Transaction_GetRemoteBankCode(t)
        //AB_Transaction_GetRemoteIban(t)
        //AB_Transaction_GetRemoteBic(t)
      }
      t = AB_ImExporterAccountInfo_GetNextTransaction(ai);
    }
    ai = AB_ImExporterContext_GetNextAccountInfo(ctx);
  }

  AB_Job_free(job);
  AB_Job_List2_free(jl);
  AB_ImExporterContext_free(ctx);

  return 0;
}