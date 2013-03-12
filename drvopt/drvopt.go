/*
*	Printservice
*
* Copyright 2012  Samsung Electronics Co., Ltd

* Licensed under the Flora License, Version 1.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://floralicense.org/license/

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

package main

import (
	"./drv"
	"flag"
	"fmt"
	"os"
	"runtime/pprof"
)

var (
	infile    string
	outfile   string
	show_help bool
	id        string
	cpuprofile string
	memprofile string
)

func init() {
	flag.StringVar(&infile, "i", "ppdi.drv", "input drv file")
	flag.StringVar(&outfile, "o", "stripped.drv", "output file")
	flag.StringVar(&cpuprofile, "p", "", "write cpu profile to file")
	flag.StringVar(&memprofile, "m", "", "write memory profile to this file")
}

func main() {
	flag.Parse()

	if cpuprofile != "" {
        f, err := os.Create(cpuprofile)
        if err != nil {
            fmt.Print(err)
        }
        pprof.StartCPUProfile(f)
        defer pprof.StopCPUProfile()
    }
    
    if memprofile != "" {
        f, err := os.Create(memprofile)
        if err != nil {
            fmt.Print(err)
        }
        pprof.WriteHeapProfile(f)
        f.Close()
    }
    
	drv, err := drv.NewDrv(infile)
	if err != nil {
		fmt.Println(err)
		return
	}

	fd, err := os.Create(outfile)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer fd.Close()
	
	_, err = fd.WriteString(drv.String())
	if err != nil {
		fmt.Println(err)
		return
	}

	return
}
