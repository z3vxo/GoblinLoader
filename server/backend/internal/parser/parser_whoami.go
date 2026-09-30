package parser

import (
	"bytes"
	"fmt"
	"strings"
)

const whoamiEndSig = 0xFFFFFFFF

type WhoamiPrivilege struct {
	Name   string
	Status uint32
}

type Whoami struct {
	Name       string
	Domain     string
	SID        string
	Privileges []WhoamiPrivilege
}

func ParseWhoami(data []byte) (*Whoami, error) {
	r := NewReader(bytes.NewReader(data))

	w := &Whoami{}
	w.Name = r.ReadString()
	w.Domain = r.ReadString()
	w.SID = r.ReadString()
	if r.Err() != nil {
		return nil, r.Err()
	}

	for {
		n := r.Read4()
		if r.Err() != nil {
			return nil, r.Err()
		}
		if n == whoamiEndSig {
			break
		}

		name := r.ReadN(int(n))
		status := r.Read4()
		if r.Err() != nil {
			return nil, r.Err()
		}

		w.Privileges = append(w.Privileges, WhoamiPrivilege{
			Name:   string(name),
			Status: status,
		})
	}

	return w, nil
}


func privStatusLabel(s uint32) string {
	switch s {
	case 0:
		return "Disabled"
	case 1:
		return "Enabled"
	case 2:
		return "Enabled By Default"
	case 3:
		return "Removed"
	default:
		return fmt.Sprintf("0x%x", s)
	}
}

// privilegeDescriptions mirrors the "Description" column of whoami /priv.
// Unknown privileges fall back to an empty cell.
var privilegeDescriptions = map[string]string{
	"SeCreateTokenPrivilege":                    "Create a token object",
	"SeAssignPrimaryTokenPrivilege":             "Replace a process level token",
	"SeLockMemoryPrivilege":                     "Lock pages in memory",
	"SeIncreaseQuotaPrivilege":                  "Adjust memory quotas for a process",
	"SeMachineAccountPrivilege":                 "Add workstations to domain",
	"SeTcbPrivilege":                            "Act as part of the operating system",
	"SeSecurityPrivilege":                       "Manage auditing and security log",
	"SeTakeOwnershipPrivilege":                  "Take ownership of files or other objects",
	"SeLoadDriverPrivilege":                     "Load and unload device drivers",
	"SeSystemProfilePrivilege":                  "Profile system performance",
	"SeSystemtimePrivilege":                     "Change the system time",
	"SeProfileSingleProcessPrivilege":           "Profile a single process",
	"SeIncreaseBasePriorityPrivilege":           "Increase scheduling priority",
	"SeCreatePagefilePrivilege":                 "Create a pagefile",
	"SeCreatePermanentPrivilege":                "Create permanent shared objects",
	"SeBackupPrivilege":                         "Back up files and directories",
	"SeRestorePrivilege":                        "Restore files and directories",
	"SeShutdownPrivilege":                       "Shut down the system",
	"SeDebugPrivilege":                          "Debug programs",
	"SeAuditPrivilege":                          "Generate security audits",
	"SeSystemEnvironmentPrivilege":              "Modify firmware environment values",
	"SeChangeNotifyPrivilege":                   "Bypass traverse checking",
	"SeRemoteShutdownPrivilege":                 "Force shutdown from a remote system",
	"SeUndockPrivilege":                         "Remove computer from docking station",
	"SeSyncAgentPrivilege":                      "Synchronize directory service data",
	"SeEnableDelegationPrivilege":               "Enable computer and user accounts to be trusted for delegation",
	"SeManageVolumePrivilege":                   "Perform volume maintenance tasks",
	"SeImpersonatePrivilege":                    "Impersonate a client after authentication",
	"SeCreateGlobalPrivilege":                   "Create global objects",
	"SeTrustedCredManAccessPrivilege":           "Access Credential Manager as a trusted caller",
	"SeRelabelPrivilege":                        "Modify an object label",
	"SeIncreaseWorkingSetPrivilege":             "Increase a process working set",
	"SeTimeZonePrivilege":                       "Change the time zone",
	"SeCreateSymbolicLinkPrivilege":             "Create symbolic links",
	"SeDelegateSessionUserImpersonatePrivilege": "Obtain an impersonation token for another user in the same session",
}

func privilegeDescription(name string) string {
	return privilegeDescriptions[name]
}

func maxInt(a, b int) int {
	if a > b {
		return a
	}
	return b
}


func FormatWhoami(w *Whoami) string {
	var b strings.Builder

	user := w.Name
	if w.Domain != "" {
		user = w.Domain + "\\" + w.Name
	}


	b.WriteString("USER INFORMATION\n")
	b.WriteString("----------------\n\n")

	nameW := maxInt(len("User Name"), len(user))
	sidW := maxInt(len("SID"), len(w.SID))

	fmt.Fprintf(&b, "%-*s %s\n", nameW, "User Name", "SID")
	fmt.Fprintf(&b, "%s %s\n", strings.Repeat("=", nameW), strings.Repeat("=", sidW))
	fmt.Fprintf(&b, "%-*s %s\n", nameW, user, w.SID)


	if len(w.Privileges) > 0 {
		b.WriteString("\nPRIVILEGES INFORMATION\n")
		b.WriteString("----------------------\n\n")

		nameW = len("Privilege Name")
		descW := len("Description")
		stateW := len("State")
		for _, p := range w.Privileges {
			nameW = maxInt(nameW, len(p.Name))
			descW = maxInt(descW, len(privilegeDescription(p.Name)))
			stateW = maxInt(stateW, len(privStatusLabel(p.Status)))
		}

		fmt.Fprintf(&b, "%-*s %-*s %s\n", nameW, "Privilege Name", descW, "Description", "State")
		fmt.Fprintf(&b, "%s %s %s\n",
			strings.Repeat("=", nameW),
			strings.Repeat("=", descW),
			strings.Repeat("=", stateW))

		for _, p := range w.Privileges {
			fmt.Fprintf(&b, "%-*s %-*s %s\n",
				nameW, p.Name,
				descW, privilegeDescription(p.Name),
				privStatusLabel(p.Status))
		}
	}

	return b.String()
}
