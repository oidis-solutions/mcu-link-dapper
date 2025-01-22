# * ********************************************************************************************************* *
# *
# * Copyright 2024 NXP
# *
# * SPDX-License-Identifier: BSD-3-Clause
# * The BSD-3-Clause license for this file can be found in the LICENSE.txt file included with this distribution
# * or at https://spdx.org/licenses/BSD-3-Clause.html#licenseText
# *
# * ********************************************************************************************************* *

$scriptPath = split-path -parent $MyInvocation.MyCommand.Definition
$projectRoot = [IO.Path]::GetFullPath((Join-Path ".\..\" (get-item $scriptPath ).parent))

if (-Not(Test-Path "$projectRoot\package.json"))
{
    Write-Output "Incorrect project root."
    exit 1
}
cd -Path "$projectRoot\src\builder"

docker-compose -f builder.yml up --force-recreate --build --remove-orphans --abort-on-container-exit

if ($LastExitCode -ne 0) {
    exit 1
}

cd -Path "$scriptPath"
