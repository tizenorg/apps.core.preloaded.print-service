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
	"io/ioutil"
	"os"
	"os/exec"
	"runtime/pprof"
)

var (
	infile		string
	outfile		*os.File
	err			error
	glob		string
	product		string
	listmod		string
	listprod	string
	show_help	bool
	cpuprofile	string
)

func init() {
	flag.StringVar(&infile, "i", "stripped.drv", "input drv file")
	flag.StringVar(&glob, "r", "", "model search pattern")
	flag.StringVar(&product, "p", "", "product search pattern")
	flag.StringVar(&listmod, "l", "", "list models")
	flag.StringVar(&listprod, "c", "", "list products")
	flag.StringVar(&cpuprofile, "f", "", "write cpu profile to file")
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

	if infile == "" {
		flag.Usage()
		return
	}

	if listmod != "" {
		drvm := drv.LoadDrvm(listmod)
		drvm.ListModels()
		return
	}

	if listprod != "" {
		drvm := drv.LoadDrvm(listprod)
		drvm.ListProducts()
		return
	}

	drvm := drv.LoadDrvm(infile)

	if outfile == nil {
		outfile, err = ioutil.TempFile("/tmp", "DRV")
		if err != nil {
			fmt.Println(err)
			return
		}
	}
	defer outfile.Close()
	defer os.Remove(outfile.Name())

	switch {
	case glob != "":
		if drvm.PrintModel(glob, outfile) != nil {
			drvm.PrintProduct(glob, outfile)
		}
	case product != "":
		if drvm.PrintProduct(product, outfile) != nil {
			drvm.PrintModel(product, outfile)
		}
	}

	cmd := exec.Command("ppdc", "-v", "-d", ".", outfile.Name())
	out,err := cmd.Output()
	if err != nil {
		fmt.Printf("cannot create ppd for model %s : %s\n", glob, err)
	}

	fmt.Printf("%s",out)
	return
}
